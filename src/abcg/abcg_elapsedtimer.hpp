/**
 * @file abcg_elapsedtimer.hpp
 * @brief Header file of abcg::ElapsedTimer.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_ELAPSEDTIMER_HPP_
#define ABCG_ELAPSEDTIMER_HPP_

#include <chrono>

namespace abcg {
class ElapsedTimer;
} // namespace abcg

/**
 * @brief Represents a timer that calculates how much time has elapsed between
 * two events.
 *
 * The timer is based on the monotonic clock `std::chrono::steady_clock`.
 */
class abcg::ElapsedTimer {
public:
  [[nodiscard]] double elapsed() const;
  double restart();

private:
  using clock = std::chrono::steady_clock;

  clock::time_point start{clock::now()};
};

#endif