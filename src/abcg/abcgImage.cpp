/**
 * @file abcgImage.cpp
 * @brief Definition of texture loading helper functions.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#include "abcgImage.hpp"

#include <cppitertools/itertools.hpp>
#include <span>
#include <vector>

void abcg::flipHorizontally(gsl::not_null<SDL_Surface *> const surface) {
  auto const width{
      static_cast<std::size_t>(surface->w * surface->format->BytesPerPixel)};
  auto const height{static_cast<std::size_t>(surface->h)};
  std::span const pixels{static_cast<std::byte *>(surface->pixels),
                         width * height};

  // Row of pixels for the swap
  std::vector<std::byte> pixelRow(width, std::byte{});

  // For each row
  for (auto const rowIndex : iter::range(height)) {
    auto const rowStart{width * rowIndex};
    auto const rowEnd{rowStart + width - 1};
    // For each RGB triplet of this row
    // C++23: for (auto tripletStart : iter::range(0uz, width, 3uz)) {
    for (auto const tripletStart : iter::range<std::size_t>(0, width, 3)) {
      pixelRow.at(tripletStart + 0) = pixels[rowEnd - tripletStart - 2];
      pixelRow.at(tripletStart + 1) = pixels[rowEnd - tripletStart - 1];
      pixelRow.at(tripletStart + 2) = pixels[rowEnd - tripletStart - 0];
    }
    memcpy(pixels.subspan(rowStart).data(), pixelRow.data(), width);
  }
}

void abcg::flipVertically(gsl::not_null<SDL_Surface *> const surface) {
  auto const width{
      static_cast<std::size_t>(surface->w * surface->format->BytesPerPixel)};
  auto const height{static_cast<std::size_t>(surface->h)};
  std::span const pixels{static_cast<std::byte *>(surface->pixels),
                         width * height};

  // Row of pixels for the swap
  std::vector<std::byte> pixelRow(width, std::byte{});

  // If height is odd, don't need to swap middle row
  for (auto const halfHeight{height / 2};
       auto const rowIndex : iter::range(halfHeight)) {
    auto const rowStartFromTop{width * rowIndex};
    auto const rowStartFromBottom{width * (height - rowIndex - 1)};
    memcpy(pixelRow.data(), pixels.subspan(rowStartFromTop).data(), width);
    memcpy(pixels.subspan(rowStartFromTop).data(),
           pixels.subspan(rowStartFromBottom).data(), width);
    memcpy(pixels.subspan(rowStartFromBottom).data(), pixelRow.data(), width);
  }
}