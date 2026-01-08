/**
 * @file textureblit.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef TEXTUREBLIT_HPP_
#define TEXTUREBLIT_HPP_

#include <abcgOpenGLExternal.hpp>
#include <glm/glm.hpp>

#include <string_view>

class TextureBlit {
public:
  TextureBlit() = default;
  ~TextureBlit() { destroy(); }

  TextureBlit(TextureBlit const &) = delete;
  TextureBlit &operator=(TextureBlit const &) = delete;
  TextureBlit(TextureBlit &&) = delete;
  TextureBlit &operator=(TextureBlit &&) = delete;

  void blit(GLuint colorTexture, glm::vec4 tintColor = glm::vec4{1.0});

private:
  static constexpr std::string_view kVertexShaderPath{"shaders/blit.vert"};
  static constexpr std::string_view kFragmentShaderPath{"shaders/blit.frag"};

  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_program{};

  GLint m_colorTextureLocation{};
  GLint m_tintColorLocation{};

  void create();
  void destroy();
};

#endif