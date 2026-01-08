/**
 * @file abcgImage.cpp
 * @brief Definition of image manipulation helper functions.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2026 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#include "abcgImage.hpp"

#include <cppitertools/itertools.hpp>
#include <fmt/format.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "abcgException.hpp"

/**
 * @brief Loads an image from disk using stb_image.
 *
 * Loads the file at the given path and decodes it into raw pixel data.
 * Optionally forces a specific number of channels.
 *
 * @param path Filesystem path to the image file.
 *
 * @param forceChannels Number of channels to force (e.g., 3 or 4). Pass 0
 * to preserve the file's original channels (default).
 *
 * @throw abcg::RuntimeError If the file cannot be opened or decoded.
 *
 * @return A fully populated Image structure containing width, height, channel
 * count, and pixel data.
 */
abcg::Image abcg::loadImage(std::string_view path, int forceChannels) {
  int w{}, h{}, c{};
  auto *pixels{stbi_load(path.data(), &w, &h, &c, forceChannels)};
  if (!pixels) {
    throw abcg::RuntimeError(
        fmt::format("Failed to load texture file {}", path));
  }

  Image img{};
  img.width = w;
  img.height = h;
  img.channels = (forceChannels != 0 ? forceChannels : c);
  img.data.assign(pixels, pixels + (w * h * img.channels));
  stbi_image_free(pixels);

  return img;
}

/**
 * @brief Flips an image horizontally.
 *
 * Flips the image horizontally by swapping pixels across the vertical axis.
 * This mirrors each scanline left-to-right in-place.
 *
 * @param img Image data loaded with abcg::loadImage.
 */
void abcg::flipHorizontally(Image &img) {
  auto const rowStride{img.width * img.channels};
  for (auto const y : iter::range(img.height)) {
    auto *row{img.data.data() + y * rowStride};
    for (auto const x : iter::range(img.width / 2)) {
      auto *left{row + x * img.channels};
      auto *right{row + (img.width - 1 - x) * img.channels};
      for (auto const c : iter::range(img.channels)) {
        std::swap(left[c], right[c]);
      }
    }
  }
}

/**
 * @brief Flips an image vertically.
 *
 * Flips the image vertically by swapping entire scanlines.
 * This mirrors the image top-to-bottom in-place.
 *
 * @param img Image data loaded with abcg::loadImage.
 */
void abcg::flipVertically(Image &img) {
  auto const rowStride{img.width * img.channels};
  for (auto const y : iter::range(img.height / 2)) {
    auto *top{img.data.data() + y * rowStride};
    auto *bottom{img.data.data() + (img.height - 1 - y) * rowStride};
    std::swap_ranges(top, top + rowStride, bottom);
  }
}