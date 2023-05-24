// Copyright 2023 Ekumen, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef BELUGA_MOTION_OMNIDIRECTIONAL_DRIVE_MODEL_HPP
#define BELUGA_MOTION_OMNIDIRECTIONAL_DRIVE_MODEL_HPP

#include <optional>
#include <random>

#include <sophus/se2.hpp>
#include <sophus/so2.hpp>

/**
 * \file
 * \brief Implementation of an omnidirectional drive odometry motion model.
 */

namespace beluga {

/// Parameters to construct an OmnidirectionalDriveModel instance.
struct OmnidirectionalDriveModelParam {
  /// Rotational noise from rotation
  /**
   * How much rotational noise is generated by the relative rotation between the last two odometry updates.
   * Also known as `alpha1`.
   */
  double rotation_noise_from_rotation;
  /// Rotational noise from translation
  /**
   * How much rotational noise is generated by the relative translation between the last two odometry updates.
   * Also known as `alpha2`.
   */
  double rotation_noise_from_translation;
  /// Translational noise from translation
  /**
   * How much translational in longitudinal noise is generated by the relative translation between
   * the last two odometry updates.
   * Also known as `alpha3`.
   */
  double translation_noise_from_translation;
  /// Translational noise from rotation
  /**
   * How much translational noise is generated by the relative rotation between the last two odometry updates.
   * Also known as `alpha4`.
   */
  double translation_noise_from_rotation;
  /// Translational strafe noise from translation
  /**
   * How much translational noise in strafe is generated by the relative translation between the last two
   * odometry updates.
   * Also known as `alpha5`.
   */
  double strafe_noise_from_translation;

  /// Distance threshold to detect in-place rotation.
  double distance_threshold = 0.01;
};

/// Sampled odometry model for an omnidirectional drive.
/**
 * This class implements the OdometryMotionModelInterface2d and satisfies \ref MotionModelPage.
 *
 * \tparam Mixin The mixed-in type with no particular requirements.
 */
template <class Mixin>
class OmnidirectionalDriveModel : public Mixin {
 public:
  /// Update type of the motion model, same as the state_type in the odometry model.
  using update_type = Sophus::SE2d;
  /// State type of a particle.
  using state_type = Sophus::SE2d;

  /// Parameter type that the constructor uses to configure the motion model.
  using param_type = OmnidirectionalDriveModelParam;

  /// Constructs an OmnidirectionalDriveModel instance.
  /**
   * \tparam ...Args Arguments types for the remaining mixin constructors.
   * \param params Parameters to configure this instance.
   *  See beluga::OmnidirectionalDriveModelParam for details.
   * \param ...args Arguments that are not used by this part of the mixin, but by others.
   */
  template <class... Args>
  explicit OmnidirectionalDriveModel(const param_type& params, Args&&... args)
      : Mixin(std::forward<Args>(args)...), params_{params} {}

  /// Applies the last motion update to the given particle state.
  /**
   * \tparam Generator  A random number generator that must satisfy the
   *  [UniformRandomBitGenerator](https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator)
   *  requirements.
   * \param state The state of the particle to which the motion will be applied.
   * \param gen An uniform random bit generator object.
   */
  template <class Generator>
  [[nodiscard]] state_type apply_motion(const state_type& state, Generator& gen) const {
    static thread_local auto distribution = std::normal_distribution<double>{};
    // This is an implementation based on the same set of parameters that is used in
    // nav2's omni_motion_model. To simplify the implementation, the following
    // variable substitutions were performed:
    // - first_rotation = delta_bearing - previous_orientation
    // - second_rotation = delta_rot_hat - first_rotation
    const auto second_rotation = Sophus::SO2d{distribution(gen, rotation_params_)} * first_rotation_.inverse();
    const auto translation = Eigen::Vector2d{
        distribution(gen, translation_params_),
        -1.0 * distribution(gen, strafe_params_),
    };
    return state * Sophus::SE2d{first_rotation_, Eigen::Vector2d{0.0, 0.0}} *
           Sophus::SE2d{second_rotation, translation};
  }

  /// \copydoc OdometryMotionModelInterface2d::update_motion(const Sophus::SE2d&)
  void update_motion(const update_type& pose) final {
    if (last_pose_) {
      const auto translation = pose.translation() - last_pose_.value().translation();
      const double distance = translation.norm();
      const double distance_variance = distance * distance;

      const auto& previous_orientation = last_pose_.value().so2();
      const auto& current_orientation = pose.so2();
      const auto rotation = current_orientation * previous_orientation.inverse();

      {
        first_rotation_ =
            distance > params_.distance_threshold
                ? Sophus::SO2d{std::atan2(translation.y(), translation.x())} * previous_orientation.inverse()
                : Sophus::SO2d{0.0};

        rotation_params_ = DistributionParam{
            rotation.log(), std::sqrt(
                                params_.rotation_noise_from_rotation * rotation_variance(rotation) +
                                params_.rotation_noise_from_translation * distance_variance)};
        translation_params_ = DistributionParam{
            distance, std::sqrt(
                          params_.translation_noise_from_translation * distance_variance +
                          params_.translation_noise_from_rotation * rotation_variance(rotation))};
        strafe_params_ = DistributionParam{
            0.0, std::sqrt(
                     params_.strafe_noise_from_translation * distance_variance +
                     params_.translation_noise_from_rotation * rotation_variance(rotation))};
      }
    }
    last_pose_ = pose;
  }

  /// Recovers the latest motion update.
  /**
   * \return Last motion update received by the model or an empty optional if no update was received.
   */
  [[nodiscard]] std::optional<update_type> latest_motion_update() const { return last_pose_; }

 private:
  using DistributionParam = typename std::normal_distribution<double>::param_type;

  OmnidirectionalDriveModelParam params_;
  std::optional<Sophus::SE2d> last_pose_;

  DistributionParam rotation_params_{0.0, 0.0};
  DistributionParam strafe_params_{0.0, 0.0};
  DistributionParam translation_params_{0.0, 0.0};
  Sophus::SO2d first_rotation_;

  static double rotation_variance(const Sophus::SO2d& rotation) {
    // Treat backward and forward motion symmetrically for the noise models.
    const auto flipped_rotation = rotation * Sophus::SO2d{Sophus::Constants<double>::pi()};
    const auto delta = std::min(std::abs(rotation.log()), std::abs(flipped_rotation.log()));
    return delta * delta;
  }
};

}  // namespace beluga

#endif
