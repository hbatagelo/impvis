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
#include <filesystem>
#include <span>
#include <vector>

namespace abcg {
class Image;
} // namespace abcg

/**
 * @brief Stores raw 8-bit pixel data loaded via stb_image.
 *
 * Layout:
 * - Row-major
 * - Interleaved channels
 * - size = width * height * channels
 */
class abcg::Image {
public:
  /**
   * @brief Image size and color depth.
   *
   */
  struct Dimensions {
    /** @brief Image width, in pixels. */
    std::size_t width{};
    /** @brief Image height, in pixels. */
    std::size_t height{};
    /** @brief Number of channels. */
    std::size_t channels{};
  };

  /**
   * @brief Desired channel layout for loaded image data.
   *
   * Controls how many color channels the image will have after loading.
   * When a specific format is requested, the image data is converted
   * accordingly by stb_image.
   *
   * @note When using formats other than FromFile, stb_image will
   * automatically add, drop, or replicate channels as needed.
   */
  enum class ChannelLayout : std::uint8_t {
    /** @brief Preserve the number of channels from the image file. */
    FromFile = 0,
    /** @brief Single-channel grayscale image (1 channel). */
    Gray = 1,
    /** @brief Grayscale image with alpha channel (2 channels). */
    GrayAlpha = 2,
    /** @brief RGB color image without alpha (3 channels). */
    RGB = 3,
    /** @brief RGBA color image with alpha (4 channels). */
    RGBA = 4
  };

  Image() = delete;
  explicit Image(std::filesystem::path const& path,
                 ChannelLayout layout = Image::ChannelLayout::FromFile);

  [[nodiscard]] Dimensions const &dimensions() const noexcept;
  [[nodiscard]] std::span<std::uint8_t> data() noexcept;
  [[nodiscard]] std::span<std::uint8_t const> data() const noexcept;

  void flipVertically() noexcept;
  void flipHorizontally() noexcept;

private:
  void loadImage(std::filesystem::path const& path, ChannelLayout layout);

  std::vector<uint8_t> m_data;
  Dimensions m_dimensions;
};

#endif
