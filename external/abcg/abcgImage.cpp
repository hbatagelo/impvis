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
#include <gsl/gsl>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <span>

#include "abcgException.hpp"
#include "abcgUtil.hpp"

/**
 * @brief Constructs a new abcg::Image.
 *
 * Loads the file at the given path and decodes it into raw pixel data.
 * Optionally forces a specific number of channels.
 *
 * @param path Filesystem path to the image file.
 *
 * @param layout Channel layout. Use abcg::Image::ChannelLayout::FromFile
 * to preserve the file's original layout (default).
 *
 * @throw abcg::RuntimeError If the file cannot be opened or decoded.
 */
abcg::Image::Image(std::filesystem::path const &path, ChannelLayout layout) {
  loadImage(path, layout);
}

/**
 * @brief Returns the dimensions (width, height, channel count) of the image.
 *
 * @returns Image dimensions.
 */
abcg::Image::Dimensions const &abcg::Image::dimensions() const noexcept {
  return m_dimensions;
}

/**
 * @brief Returns a mutable span of the image data.
 *
 * @returns Image data in row-major order and interleaved (size = width *
 * height * channels).
 */
std::span<std::uint8_t> abcg::Image::data() noexcept { return m_data; }

/**
 * @brief Returns a view (immutable span) of the image data.
 *
 * @returns Image data in row-major order and interleaved (size = width *
 * height * channels).
 */
std::span<std::uint8_t const> abcg::Image::data() const noexcept {
  return m_data;
}

void abcg::Image::loadImage(std::filesystem::path const &path,
                            ChannelLayout layout) {
  using StbiDeleter = decltype(&stbi_image_free);
  using StbiPtr = std::unique_ptr<stbi_uc, StbiDeleter>;

  auto const requestedChannels{static_cast<int>(layout)};

  int width{};
  int height{};
  int channels{};
  StbiPtr rawPixels{stbi_load(pathToUtf8(path).c_str(), &width, &height,
                              &channels, requestedChannels),
                    stbi_image_free};

  if (!rawPixels) {
    throw abcg::RuntimeError(
        fmt::format("Failed to load image file {}", path.string()));
  }

  auto const actualChannels{
      (layout != ChannelLayout::FromFile ? requestedChannels : channels)};
  m_dimensions = Dimensions{.width = gsl::narrow<size_t>(width),
                            .height = gsl::narrow<size_t>(height),
                            .channels = gsl::narrow<size_t>(actualChannels)};

  auto const sizeInBytes{m_dimensions.width * m_dimensions.height *
                         m_dimensions.channels};
  std::span<stbi_uc const> const pixels{rawPixels.get(), sizeInBytes};

  m_data.assign(pixels.begin(), pixels.end());
}

/**
 * @brief Flips the image vertically.
 *
 * Flips the image vertically by swapping entire scanlines.
 * This mirrors the image top-to-bottom in-place.
 */
void abcg::Image::flipVertically() noexcept {
  auto const &[width, height, channels]{m_dimensions};
  auto const rowStride{width * channels};
  auto const halfHeight{height / 2};
  auto const img{std::span{m_data}};
  for (auto const y : iter::range(halfHeight)) {
    auto const top{img.subspan(y * rowStride, rowStride)};
    auto const bottom{img.subspan((height - 1 - y) * rowStride, rowStride)};
    std::ranges::swap_ranges(top, bottom);
  }
}

/**
 * @brief Flips the image horizontally.
 *
 * Flips the image horizontally by swapping pixels across the vertical axis.
 * This mirrors each scanline left-to-right in-place.
 *
 * @param img Image data loaded with abcg::loadImage.
 */
void abcg::Image::flipHorizontally() noexcept {
  auto const &[width, height, channels]{m_dimensions};
  auto const rowStride{width * channels};
  auto const halfWidth{width / 2};
  auto const img{std::span{m_data}};
  for (auto const y : iter::range(height)) {
    auto const row{img.subspan(y * rowStride, rowStride)};
    for (auto const x : iter::range(halfWidth)) {
      auto const left{row.subspan(x * channels, channels)};
      auto const right{row.subspan((width - 1 - x) * channels, channels)};
      std::ranges::swap_ranges(left, right);
    }
  }
}
