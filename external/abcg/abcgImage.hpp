/**
 * @file abcgImage.hpp
 * @brief Declaration of image manipulation helper functions.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2026 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_IMAGE_HPP_
#define ABCG_IMAGE_HPP_

#include <cstdint>
#include <string_view>
#include <vector>

namespace abcg {
struct Image;
}

/**
 * @brief Stores raw 8-bit pixel data loaded via stb_image.
 */
struct abcg::Image {
  /** @brief Image width. */
  int width{};
  /** @brief Image height. */
  int height{};
  /** @brief Number of channels (typically 3 or 4). */
  int channels{};
  /** @brief Image data in row-major order and interleaved
   * (size = width * height * channel) */
  std::vector<uint8_t> data;
};

namespace abcg {
Image loadImage(std::string_view path, int forceChannels = 0);
void flipHorizontally(Image &img);
void flipVertically(Image &img);
} // namespace abcg

#endif
