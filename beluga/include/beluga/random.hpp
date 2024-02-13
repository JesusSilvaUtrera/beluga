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

#ifndef BELUGA_RANDOM_HPP
#define BELUGA_RANDOM_HPP

/**
 * \file
 * \brief Includes all Beluga random distributions.
 */

#include <beluga/random/multivariate_normal_distribution.hpp>
#include <beluga/random/uniform_free_space_grid_distribution.hpp>

/**
 * \page RandomStateDistributionPage Beluga named requirements: RandomStateDistribution
 * A RandomStateDistribution is a function object returning random states according to a probability
 * density function p(x) or a discrete probability distribution P(xi).
 *
 * The requirements are very similar to the standard
 * [RandomNumberDistribution](https://en.cppreference.com/w/cpp/named_req/RandomNumberDistribution),
 * the main difference being that the result type does not need to be an arithmetic type.
 *
 * \section RandomStateDistributionRequirements Requirements
 * The type D satisfies _RandomStateDistribution_ if:
 * - D satisfies [CopyConstructible](https://en.cppreference.com/w/cpp/named_req/CopyConstructible)
 * - D satisfies [CopyAssignable](https://en.cppreference.com/w/cpp/named_req/CopyAssignable)
 *
 * Given:
 * - T, the type named by `D::result_type`
 * - P, the type named by `D::param_type`, which
 *   - satisfies [CopyConstructible](https://en.cppreference.com/w/cpp/named_req/CopyConstructible)
 *   - satisfies [CopyAssignable](https://en.cppreference.com/w/cpp/named_req/CopyAssignable)
 *   - satisfies [EqualityComparable](https://en.cppreference.com/w/cpp/named_req/EqualityComparable)
 *   - has a constructor taking identical arguments as each of the constructors of D that take arguments corresponding
 *     to the distribution parameters
 *   - has a member function with the identical name, type, and semantics, as every member function of D that returns a
 *     parameter of the distribution
 *   - declares a member typedef `using distribution_type = D;`
 * - d, a value of type D
 * - x and y, (possibly const) values of type D
 * - p, a (possibly const) value of type P
 * - g, lvalue of a type satisfying
 *   [UniformRandomBitGenerator](https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator)
 *
 * The following expressions must be valid and have their specified effects:
 * - D::result_type
 * - D::param_type
 * - D() creates a distribution indistinguishable from any other default-constructed D
 * - D(p) creates a distribution indistinguishable from D constructed directly from the values used to construct p
 * - d.reset() resets the internal state of the distribution
 * - d.param() returns p such that D(p).param() == p
 * - d.param(p) sets a new parameter set
 * - d(g) returns random objects according to the distribution parametrized by d.param()
 * - d(g, p) returns random objects according to the distribution parametrized by p
 * - x == y returns true if x.param() == y.param() and future infinite sequences of values that
 *   would be generated by repeated invocations of x(g1) and y(g2) would be equal as long as g1 == g2
 * - x != y returns !(x == y)
 *
 * \section RandomStateDistributionLinks See also
 * - beluga::MultivariateNormalDistribution
 */

#endif
