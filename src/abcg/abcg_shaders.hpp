/**
 * @file abcg_shaders.hpp
 * @brief Declaration of helper functions for building shaders and programs.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_SHADERS_HPP_
#define ABCG_SHADERS_HPP_

#include <abcg_external.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace abcg {
struct Shaders;
}; // namespace abcg

/**
 * @brief Namespace for OpenGL-related classes and functions.
 */
namespace abcg::opengl {
[[nodiscard]] GLuint createProgram(Shaders const &shaders,
                                   bool throwOnError = true);
[[nodiscard]] std::vector<GLuint> triggerCompile(Shaders const &shaders);
[[nodiscard]] bool checkCompile(std::vector<GLuint> const &shaderIDs,
                                bool throwOnError = true);
[[nodiscard]] GLuint triggerLink(std::vector<GLuint> const &shaderIDs,
                                 bool throwOnError = true);
[[nodiscard]] bool checkLink(GLuint shaderProgram, bool throwOnError = true);
} // namespace abcg::opengl

/**
 * @brief Simple structure of strings containing paths or source codes of
 * shaders to be compiled.
 *
 * This is used as input to abcg::opengl::createProgram and
 * abcg::opengl::triggerCompile.
 *
 * Mixing paths and source codes is supported. For example,
 * abcg::Shaders::vertexShader can contain the path to the vertex shader file,
 * while abcg::Shaders::fragmentShader can contain the source code of the
 * fragment shader. If a string is empty, the correspoding shader is not used.
 *
 * @sa abcg::opengl::createProgram.
 * @sa abcg::opengl::triggerCompile.
 */
struct abcg::Shaders {
  /**
   * @brief String containing the path or source code of a vertex shader.
   */
  std::string vertexShader{};
  /**
   * @brief String containing the path or source code of a fragment shader.
   */
  std::string fragmentShader{};
  /**
   * @brief String containing the path or source code of a geometry shader.
   */
  std::string geometryShader{};
  /**
   * @brief String containing the path or source code of a tessellation control
   * shader.
   *
   * This is not used when the OpenGL context is created with
   * abcg::OpenGLProfile::ES.
   */
  std::string tessControlShader{};
  /**
   * @brief String containing the path or source code of a tessellation
   * evaluation shader.
   *
   * This is not used when the OpenGL context is created with
   * abcg::OpenGLProfile::ES.
   */
  std::string tessEvalShader{};
  /**
   * @brief String containing the path or source code of a computer shader.
   *
   * This is not used when the OpenGL context is created with
   * abcg::OpenGLProfile::ES.
   */
  std::string computeShader{};
};

#endif
