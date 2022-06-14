/**
 * @file abcg_image.hpp
 * @brief Declaration of texture loading helper functions.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_IMAGE_HPP_
#define ABCG_IMAGE_HPP_

#include <abcg_external.hpp>
#include <array>
#include <string_view>

namespace abcg::opengl {
[[nodiscard]] GLuint loadTexture(std::string_view path,
                                 bool generateMipmaps = true,
                                 bool flipUpsideDown = true);
[[nodiscard]] GLuint loadCubemap(std::array<std::string, 6> paths,
                                 bool generateMipmaps = true,
                                 bool rightHandedSystem = true);
} // namespace abcg::opengl

#endif
