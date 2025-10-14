// Copyright 2024 Ekumen, Inc.
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

#ifndef BELUGA_ACTIONS_REWEIGHT_HPP
#define BELUGA_ACTIONS_REWEIGHT_HPP

#include <algorithm>
#include <execution>

#include <beluga/type_traits/particle_traits.hpp>
#include <beluga/views/likelihoods.hpp>
#include <beluga/views/particles.hpp>

#include <range/v3/action/action.hpp>
#include <range/v3/algorithm/max_element.hpp>
#include <range/v3/view/common.hpp>
#include <range/v3/view/zip.hpp>

/**
 * \file
 * \brief Implementation of the reweight range adaptor object.
 */

namespace beluga::actions {

namespace detail {

/// Implementation detail for a reweight range adaptor object.
struct reweight_base_fn {
  /// Overload that implements the reweight algorithm.
  /**
   * \tparam ExecutionPolicy An [execution policy](https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t).
   * \tparam Range An [input range](https://en.cppreference.com/w/cpp/ranges/input_range) of particles.
   * \tparam Model A callable that can compute the importance weight given a particle state.
   * \param policy The execution policy to use.
   * \param range An existing range of particles to apply this action to.
   * \param model A callable instance to compute the weights given the particle states.
   *
   * For each particle, we multiply the current weight by the new importance weight to accumulate information from
   * sensor updates.
   */
  template <
      class ExecutionPolicy,
      class Range,
      class Model,
      std::enable_if_t<std::is_execution_policy_v<std::decay_t<ExecutionPolicy>>, int> = 0,
      std::enable_if_t<ranges::range<Range>, int> = 0,
      // This overload is disabled if `Model` is a range.
      std::enable_if_t<!ranges::range<Model>, int> = 0>
  constexpr auto operator()(ExecutionPolicy&& policy, Range& range, Model model) const -> Range& {
    static_assert(beluga::is_particle_range_v<Range>);
    auto states = range | beluga::views::states | ranges::views::common;
    auto weights = range | beluga::views::weights | ranges::views::common;
    std::transform(
        policy,               //
        std::begin(states),   //
        std::end(states),     //
        std::begin(weights),  //
        std::begin(weights),  //
        [model = std::move(model)](const auto& s, auto w) { return w * model(s); });
    return range;
  }

  /// Overload that takes a likelihood range and directly applies it
  template <
      class ExecutionPolicy,
      class Range,
      class LikelihoodRange,
      std::enable_if_t<std::is_execution_policy_v<std::decay_t<ExecutionPolicy>>, int> = 0,
      std::enable_if_t<ranges::range<LikelihoodRange>, int> = 0>
  constexpr auto operator()(ExecutionPolicy&& policy, Range& range, const LikelihoodRange& likelihoods) const
      -> Range& {
    static_assert(beluga::is_particle_range_v<Range>);
    auto zipped_view = ranges::views::zip(range, likelihoods);
    std::for_each(policy, std::begin(zipped_view), std::end(zipped_view), [](auto&& tuple) {
      auto& particle = std::get<0>(tuple);
      const auto& likelihood = std::get<1>(tuple);
      beluga::weight(particle) *= likelihood;
    });
    return range;
  }

  /// Overload that re-orders arguments from a view closure.
  template <
      class Range,
      class Model,
      class ExecutionPolicy,
      std::enable_if_t<ranges::range<Range>, int> = 0,
      std::enable_if_t<std::is_execution_policy_v<ExecutionPolicy>, int> = 0>
  constexpr auto operator()(Range&& range, Model model, ExecutionPolicy policy) const -> Range& {
    return (*this)(std::move(policy), std::forward<Range>(range), std::move(model));
  }

  /// Overload that returns a view closure to compose with other views.
  template <
      class ExecutionPolicy,
      class ThirdArg,
      std::enable_if_t<std::is_execution_policy_v<ExecutionPolicy>, int> = 0>
  constexpr auto operator()(ExecutionPolicy policy, ThirdArg third) const {
    return ranges::make_action_closure(ranges::bind_back(reweight_base_fn{}, std::move(third), std::move(policy)));
  }
};

/// Implementation detail for a reweight range adaptor object with a default execution policy.
struct reweight_fn : public reweight_base_fn {
  using reweight_base_fn::operator();

  /// Overload that defines a default execution policy.
  template <
      class Range,
      class Model,
      std::enable_if_t<ranges::range<Range>, int> = 0,
      std::enable_if_t<!ranges::range<Model>, int> = 0>
  constexpr auto operator()(Range&& range, Model model) const -> Range& {
    return (*this)(std::execution::seq, std::forward<Range>(range), std::move(model));
  }

  /// Overload that returns a view closure to compose with other views.
  template <class Model, std::enable_if_t<!ranges::range<Model>, int> = 0>
  constexpr auto operator()(Model model) const {
    return ranges::make_action_closure(ranges::bind_back(reweight_fn{}, std::move(model)));
  }

  /// Overload that defines a default execution policy for reweighting with a likelihood range instead of a model.
  template <
      class Range,
      class LikelihoodRange,
      std::enable_if_t<ranges::range<Range>, int> = 0,
      std::enable_if_t<ranges::range<LikelihoodRange>, int> = 0>
  constexpr auto operator()(Range&& range, const LikelihoodRange& likelihoods) const -> Range& {
    return (*this)(std::execution::seq, std::forward<Range>(range), likelihoods);
  }

  /// Overload that returns a view closure for reweighting with a likelihood range instead of a model.
  template <class LikelihoodRange, std::enable_if_t<ranges::range<LikelihoodRange>, int> = 0>
  constexpr auto operator()(const LikelihoodRange& likelihoods) const {
    return ranges::make_action_closure(ranges::bind_back(reweight_fn{}, likelihoods));
  }
};

}  // namespace detail

/// [Range adaptor object](https://en.cppreference.com/w/cpp/named_req/RangeAdaptorObject) that
/// can update the weights in a particle range using a sensor model.
/**
 * This action updates particle weights by importance weight multiplication.
 * These importance weights are computed by a given measurement likelihood
 * function (or sensor model) for current particle states.
 */
inline constexpr detail::reweight_fn reweight;

}  // namespace beluga::actions

#endif
