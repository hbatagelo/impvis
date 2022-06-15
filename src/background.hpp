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

#include "abcgOpenGL.hpp"

class Background {
public:
  void onCreate();
  void onPaint(GLuint renderTexture) const;
  void onResize(glm::ivec2 const &size);
  void onDestroy();

private:
  GLuint m_FBO{};
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_program{};

  GLint m_resolutionLocation{};

  glm::vec2 m_resolution{0.0f};
};

#endif