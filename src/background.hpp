/**
 * @file background.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef BACKGROUND_HPP_
#define BACKGROUND_HPP_

#include <abcgOpenGL.hpp>

class Background {
public:
  void onCreate();
  void onPaint(GLuint renderTexture);
  void onResize(glm::ivec2 size);
  void onDestroy();

private:
  static constexpr std::string_view kVertexShaderPath{
      "shaders/radialgradient.vert"};
  static constexpr std::string_view kFragmentShaderPath{
      "shaders/radialgradient.frag"};

  GLuint m_FBO{};
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_program{};

  GLint m_resolutionLocation{};

  glm::vec2 m_resolution{};

  bool m_needsRedraw{true};
};

#endif