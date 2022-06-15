/**
 * @file abcgOpenGLImage.hpp
 * @brief Declaration of OpenGL texture loading helper functions.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_OPENGL_IMAGE_HPP_
#define ABCG_OPENGL_IMAGE_HPP_

#include "abcgOpenGLExternal.hpp"

#include <array>
#include <string_view>

namespace abcg {
[[nodiscard]] GLuint loadOpenGLTexture(std::string_view path,
                                       bool generateMipmaps = true,
                                       bool flipUpsideDown = true);
[[nodiscard]] GLuint loadOpenGLCubemap(std::array<std::string, 6> paths,
                                       bool generateMipmaps = true,
                                       bool rightHandedSystem = true);
} // namespace abcg

#endif
