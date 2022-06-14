/**
 * @file textureblit.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#include <fmt/core.h>

#include "abcg_application.hpp"
#include "textureblit.hpp"

void TextureBlit::initializeGL() {
  // Create shader program
  auto const *vertexShaderFilename{"shaders/textureblit.vert"};
  auto const *fragmentShaderFilename{"shaders/textureblit.frag"};
  auto const &assetsPath{abcg::Application::getAssetsPath()};
  m_program = abcg::opengl::createProgram(
      {assetsPath + vertexShaderFilename, assetsPath + fragmentShaderFilename});

  // Create VBO
  abcg::glGenBuffers(1, &m_VBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

  struct Vertex {
    glm::vec2 position{};
  };
  std::array const vertices{
      Vertex{.position = {-1, +1}}, Vertex{.position = {-1, -1}},
      Vertex{.position = {+1, +1}}, Vertex{.position = {+1, -1}}};
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(),
                     GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Create and bind VAO
  abcg::glGenVertexArrays(1, &m_VAO);
  abcg::glBindVertexArray(m_VAO);

  // Set up vertex attributes
  auto const setUpVertexAttribute{[&](auto name, auto size, intptr_t offset) {
    auto const location{abcg::glGetAttribLocation(m_program, name)};
    if (location >= 0) {
      abcg::glEnableVertexAttribArray(static_cast<GLuint>(location));
      abcg::glVertexAttribPointer(static_cast<GLuint>(location), size, GL_FLOAT,
                                  GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void *>(offset)); // NOLINT
    } else {
      throw abcg::RunTimeError(fmt::format("Failed to find attribute {} in {}",
                                           name, vertexShaderFilename));
    }
  }};
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  setUpVertexAttribute("inPosition", 2, 0);

  // End of binding
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  abcg::glBindVertexArray(0);

  // Save location of uniform variables
  m_resolutionLocation = abcg::glGetUniformLocation(m_program, "uResolution");
}

void TextureBlit::draw(GLuint texture) const {
  abcg::glDisable(GL_DEPTH_TEST);

  abcg::glUseProgram(m_program);

  abcg::glUniform2fv(m_resolutionLocation, 1, &m_resolution.x);

  abcg::glBindVertexArray(m_VAO);
  abcg::glActiveTexture(GL_TEXTURE0);
  abcg::glBindTexture(GL_TEXTURE_2D, texture);

  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  abcg::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  abcg::glBindTexture(GL_TEXTURE_2D, 0);
  abcg::glBindVertexArray(0);

  abcg::glUseProgram(0);
}

void TextureBlit::resizeGL(int width, int height) {
  m_resolution = {width, height};
}

void TextureBlit::terminateGL() {
  abcg::glDeleteVertexArrays(1, &m_VAO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteProgram(m_program);
}