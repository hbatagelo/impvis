/**
 * @file textureblit.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#include <fmt/core.h>

#include "abcgOpenGL.hpp"
#include "textureblit.hpp"

void TextureBlit::onCreate() {
  auto const *vertexShaderPath{"shaders/textureblit.vert"};
  auto const *fragmentShaderPath{"shaders/textureblit.frag"};
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
    auto const location{abcg::glGetAttribLocation(m_program, name)};
    if (location >= 0) {
      abcg::glEnableVertexAttribArray(gsl::narrow<GLuint>(location));
      abcg::glVertexAttribPointer(gsl::narrow<GLuint>(location), size, GL_FLOAT,
                                  GL_FALSE, sizeof(glm::vec2),
                                  reinterpret_cast<void *>(offset)); // NOLINT
    } else {
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

void TextureBlit::onPaint(GLuint texture) const {
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

void TextureBlit::onResize(glm::ivec2 const &size) { m_resolution = size; }

void TextureBlit::onDestroy() {
  abcg::glDeleteVertexArrays(1, &m_VAO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteProgram(m_program);
}