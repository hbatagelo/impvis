/**
 * @file background.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#include <fmt/core.h>

#include "abcg_application.hpp"
#include "background.hpp"

void Background::initializeGL() {
  // Create FBO
  abcg::glGenFramebuffers(1, &m_FBO);

  // Create shader program
  auto const *vertexShaderPath{"shaders/radialgradient.vert"};
  auto const *fragmentShaderPath{"shaders/radialgradient.frag"};
  auto const &assetsPath{abcg::Application::getAssetsPath()};
  m_program = abcg::opengl::createProgram(
      {assetsPath + vertexShaderPath, assetsPath + fragmentShaderPath});

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
                                           name, vertexShaderPath));
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

void Background::paintGL(GLuint renderTexture) const {
  if (renderTexture > 0) {
    abcg::glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    abcg::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, renderTexture, 0);
  }

  abcg::glDisable(GL_DEPTH_TEST);

  abcg::glUseProgram(m_program);

  abcg::glUniform2fv(m_resolutionLocation, 1, &m_resolution.x);

  abcg::glBindVertexArray(m_VAO);

  abcg::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  abcg::glBindVertexArray(0);

  abcg::glUseProgram(0);

  if (renderTexture > 0) {
    abcg::glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

void Background::resizeGL(int width, int height) {
  m_resolution = {width, height};
}

void Background::terminateGL() {
  abcg::glDeleteVertexArrays(1, &m_VAO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteProgram(m_program);
  abcg::glDeleteFramebuffers(1, &m_FBO);
}