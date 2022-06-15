/**
 * @file textureblit.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#ifndef TEXTUREBLIT_HPP_
#define TEXTUREBLIT_HPP_

#include "abcgOpenGL.hpp"

class TextureBlit {
public:
  void onCreate();
  void onPaint(GLuint texture) const;
  void onResize(glm::ivec2 const &size);
  void onDestroy();

private:
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_program{};

  GLint m_resolutionLocation{};

  glm::vec2 m_resolution{0.0f};
};

#endif