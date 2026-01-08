/**
 * @file rendertarget.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef RENDERTARGET_HPP_
#define RENDERTARGET_HPP_

#include <abcgOpenGLExternal.hpp>
#include <glm/glm.hpp>

#include <vector>

class RenderTarget {
public:
  struct AttachmentSpec {
    GLint internalFormat{GL_RGBA8};
    GLenum format{GL_RGBA};
    GLenum type{GL_UNSIGNED_BYTE};
  };

  explicit RenderTarget(std::vector<AttachmentSpec> const &attachments);
  ~RenderTarget() { destroy(); }

  RenderTarget(RenderTarget const &) = delete;
  RenderTarget &operator=(RenderTarget const &) = delete;
  RenderTarget(RenderTarget &&) = delete;
  RenderTarget &operator=(RenderTarget &&) = delete;

  static constexpr AttachmentSpec kRGBA8{
      .internalFormat = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE};
  static constexpr AttachmentSpec kRGBA32F{
      .internalFormat = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT};
  static constexpr AttachmentSpec kDepth24{.internalFormat =
                                               GL_DEPTH_COMPONENT24,
                                           .format = GL_DEPTH_COMPONENT,
                                           .type = GL_UNSIGNED_INT};

  void bind() const;
  static void unbind();
  void resize(glm::ivec2 size);

  [[nodiscard]] glm::ivec2 getSize() const noexcept;
  [[nodiscard]] GLuint getColorTexture(std::size_t index = 0) const;
  [[nodiscard]] GLuint getDepthTexture() const noexcept;
  [[nodiscard]] std::size_t getColorAttachmentCount() const noexcept;

private:
  GLuint m_fbo{};
  glm::ivec2 m_size{};
  std::vector<AttachmentSpec> m_specs;
  std::vector<GLuint> m_colorTextures;
  GLuint m_depthTexture{};

  void create();
  void destroy();

  void createColorTexture(AttachmentSpec const &spec);
  void createDepthTexture(AttachmentSpec const &spec);
};

#endif // RENDERTARGET_HPP_