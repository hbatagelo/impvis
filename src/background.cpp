/**
 * @file background.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#include <fmt/core.h>

#include "abcgApplication.hpp"
#include "background.hpp"

void Background::onCreate() {
  abcg::glGenFramebuffers(1, &m_FBO);

  auto const *const vertexShaderPath{"shaders/radialgradient.vert"};
  auto const *const fragmentShaderPath{"shaders/radialgradient.frag"};
  auto const &assetsPath{abcg::Application::getAssetsPath()};
  m_program =
      abcg::createOpenGLProgram({{.source = assetsPath + vertexShaderPath,
                                  .stage = abcg::ShaderStage::Vertex},
                                 {.source = assetsPath + fragmentShaderPath,
                                  .stage = abcg::ShaderStage::Fragment}});

  abcg::glGenBuffers(1, &m_VBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

  std::array const vertices{glm::vec2{-1, +1}, glm::vec2{-1, -1},
                            glm::vec2{+1, +1}, glm::vec2{+1, -1}};
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(),
                     GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  abcg::glGenVertexArrays(1, &m_VAO);
  abcg::glBindVertexArray(m_VAO);

  auto const setUpVertexAttribute{[&](auto name, auto size, intptr_t offset) {
    if (auto const location{abcg::glGetAttribLocation(m_program, name)};
        location >= 0) {
      abcg::glEnableVertexAttribArray(gsl::narrow<GLuint>(location));
      abcg::glVertexAttribPointer(gsl::narrow<GLuint>(location), size, GL_FLOAT,
                                  GL_FALSE, sizeof(glm::vec2),
                                  reinterpret_cast<void *>(offset)); // NOLINT
    } else {
      onDestroy();
      throw abcg::RuntimeError(fmt::format("Failed to find attribute {} in {}",
                                           name, vertexShaderPath));
    }
  }};
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  setUpVertexAttribute("inPosition", 2, 0);

  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  abcg::glBindVertexArray(0);

  m_resolutionLocation = abcg::glGetUniformLocation(m_program, "uResolution");
}

void Background::onPaint(GLuint renderTexture) const {
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

void Background::onResize(glm::ivec2 const &size) { m_resolution = size; }

void Background::onDestroy() {
  abcg::glDeleteVertexArrays(1, &m_VAO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteProgram(m_program);
  abcg::glDeleteFramebuffers(1, &m_FBO);
}