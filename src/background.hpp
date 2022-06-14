/**
 * @file background.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#ifndef BACKGROUND_HPP_
#define BACKGROUND_HPP_

#include "abcg.hpp"

class Background {
public:
  void initializeGL();
  void paintGL(GLuint renderTexture) const;
  void resizeGL(int width, int height);
  void terminateGL();

private:
  GLuint m_FBO{};
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_program{};

  GLint m_resolutionLocation{};

  glm::vec2 m_resolution{0.0f};
};

#endif