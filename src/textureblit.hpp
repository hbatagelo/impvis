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

#include "abcg.hpp"

class TextureBlit {
public:
  void initializeGL();
  void draw(GLuint texture) const;
  void resizeGL(int width, int height);
  void terminateGL();

private:
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_program{};

  GLint m_resolutionLocation{};

  glm::vec2 m_resolution{0.0f};
};

#endif