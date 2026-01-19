/**
 * @file textureblit.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "textureblit.hpp"

#include <abcgOpenGL.hpp>

void TextureBlit::create() {
  destroy();

  auto const &assetsPath{abcg::Application::getAssetsPath()};

  std::vector<abcg::ShaderSource> const sources{
      {.source = abcg::pathToUtf8(assetsPath /
                                  std::filesystem::path{kVertexShaderPath}),
       .stage = abcg::ShaderStage::Vertex},
      {.source = abcg::pathToUtf8(assetsPath /
                                  std::filesystem::path{kFragmentShaderPath}),
       .stage = abcg::ShaderStage::Fragment}};

  m_program = abcg::createOpenGLProgram(sources);

  abcg::glGenBuffers(1, &m_VBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

  std::array const vertices{glm::vec2{+3, -1}, glm::vec2{-1, +3},
                            glm::vec2{-1, -1}};
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(),
                     GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  abcg::glGenVertexArrays(1, &m_VAO);
  abcg::glBindVertexArray(m_VAO);

  auto const setUpVertexAttribute{[&](auto name, auto size, intptr_t offset) {
    if (auto const location{abcg::glGetAttribLocation(m_program, name)};
        location >= 0) {
      abcg::glEnableVertexAttribArray(gsl::narrow<GLuint>(location));
      // NOLINTBEGIN(*reinterpret-cast, performance-no-int-to-ptr)
      abcg::glVertexAttribPointer(gsl::narrow<GLuint>(location), size, GL_FLOAT,
                                  GL_FALSE, sizeof(glm::vec2),
                                  reinterpret_cast<void *>(offset));
      // NOLINTEND(*reinterpret-cast, performance-no-int-to-ptr)
    } else {
      destroy();
      throw abcg::RuntimeError(std::format("Failed to find attribute {} in {}",
                                           name, kVertexShaderPath));
    }
  }};
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  setUpVertexAttribute("inPosition", 2, 0);

  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  abcg::glBindVertexArray(0);

  m_colorTextureLocation =
      abcg::glGetUniformLocation(m_program, "uColorTexture");
  m_tintColorLocation = abcg::glGetUniformLocation(m_program, "uTintColor");
}

void TextureBlit::destroy() {
  if (m_VAO != 0) {
    abcg::glDeleteVertexArrays(1, &m_VAO);
    m_VAO = 0;
  }
  if (m_VBO != 0) {
    abcg::glDeleteBuffers(1, &m_VBO);
    m_VBO = 0;
  }
  if (m_program != 0) {
    abcg::glDeleteProgram(m_program);
    m_program = 0;
  }
}

void TextureBlit::blit(GLuint colorTexture, glm::vec4 tintColor) {
  if (m_program == 0) {
    create();
  }

  abcg::glUseProgram(m_program);

  abcg::glActiveTexture(GL_TEXTURE0);
  abcg::glBindTexture(GL_TEXTURE_2D, colorTexture);
  abcg::glUniform1i(m_colorTextureLocation, 0);
  abcg::glUniform4fv(m_tintColorLocation, 1, &tintColor[0]);

  abcg::glBindVertexArray(m_VAO);
  abcg::glDrawArrays(GL_TRIANGLES, 0, 3);
  abcg::glBindVertexArray(0);

  abcg::glUseProgram(0);
}