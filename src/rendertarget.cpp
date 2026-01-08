/**
 * @file rendertarget.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "rendertarget.hpp"

#include <abcgOpenGL.hpp>

namespace {

GLuint createAndBindAttachmentTexture() {
  GLuint texture{};
  abcg::glGenTextures(1, &texture);
  abcg::glBindTexture(GL_TEXTURE_2D, texture);

  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return texture;
}

} // namespace

RenderTarget::RenderTarget(std::vector<AttachmentSpec> const &attachments)
    : m_specs(attachments) {}

void RenderTarget::resize(glm::ivec2 size) {
  if (size == m_size) {
    return;
  }

  if (size.x <= 0 || size.y <= 0) {
    throw abcg::RuntimeError("Invalid render target size");
  }

  m_size = size;

  create();
}

void RenderTarget::create() {
  destroy();

  abcg::glGenFramebuffers(1, &m_fbo);
  abcg::glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

  m_colorTextures.reserve(m_specs.size());
  for (auto const &spec : m_specs) {
    if (spec.format == GL_DEPTH_COMPONENT) {
      if (m_depthTexture != 0) {
        throw abcg::RuntimeError(
            "Attempting to attach multiple depth textures");
      }
      createDepthTexture(spec);
    } else {
      createColorTexture(spec);
    }
  }

  auto const status{abcg::glCheckFramebufferStatus(GL_FRAMEBUFFER)};
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    destroy();
    throw abcg::RuntimeError(
        std::format("Framebuffer incomplete: status = 0x{:X}", status));
  }

  abcg::glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::bind() const {
  if (m_fbo == 0) {
    throw abcg::RuntimeError("Attempting to bind invalid render target");
  }
  abcg::glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

  std::vector<GLenum> drawBuffers;
  drawBuffers.reserve(m_colorTextures.size());
  for (auto const index : iter::range(m_colorTextures.size())) {
    drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + gsl::narrow<GLenum>(index));
  }

  abcg::glDrawBuffers(gsl::narrow<GLsizei>(drawBuffers.size()),
                      drawBuffers.data());
}

void RenderTarget::unbind() { abcg::glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void RenderTarget::destroy() {
  for (auto &texture : m_colorTextures) {
    if (texture != 0) {
      abcg::glDeleteTextures(1, &texture);
      texture = 0;
    }
  }
  m_colorTextures.clear();

  if (m_depthTexture != 0) {
    abcg::glDeleteTextures(1, &m_depthTexture);
    m_depthTexture = 0;
  }

  if (m_fbo != 0) {
    abcg::glDeleteFramebuffers(1, &m_fbo);
    m_fbo = 0;
  }
}

void RenderTarget::createColorTexture(AttachmentSpec const &spec) {
  auto const texture{createAndBindAttachmentTexture()};

  // Allocate texture storage
  if (spec.type == GL_FLOAT) {
#if defined(__EMSCRIPTEN__)
    // Try RGBA32F first, fall back to RGBA16F if not supported
    GLint internalFormat{spec.internalFormat};
    abcg::glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_size.x, m_size.y, 0,
                       spec.format, spec.type, nullptr);

    auto const status{abcg::glCheckFramebufferStatus(GL_FRAMEBUFFER)};
    if (status != GL_FRAMEBUFFER_COMPLETE && internalFormat == GL_RGBA32F) {
      // Fall back to half-float
      fmt::print("Using RGBA16F instead of RGBA32F for attachment #{}\n",
                 index);
      internalFormat = GL_RGBA16F;
      abcg::glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_size.x, m_size.y,
                         0, spec.format, spec.type, nullptr);
    }
#else
    abcg::glTexImage2D(GL_TEXTURE_2D, 0, spec.internalFormat, m_size.x,
                       m_size.y, 0, spec.format, spec.type, nullptr);
#endif
  } else {
    // Initialize with zero data to avoid WebGL warnings
    std::vector<std::byte> zeroData(gsl::narrow<std::size_t>(m_size.x) *
                                        gsl::narrow<std::size_t>(m_size.y) * 4,
                                    std::byte{0});
    abcg::glTexImage2D(GL_TEXTURE_2D, 0, spec.internalFormat, m_size.x,
                       m_size.y, 0, spec.format, spec.type, zeroData.data());
  }

  abcg::glBindTexture(GL_TEXTURE_2D, 0);

  abcg::glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0 +
                                   gsl::narrow<GLenum>(m_colorTextures.size()),
                               GL_TEXTURE_2D, texture, 0);

  m_colorTextures.push_back(texture);
}

void RenderTarget::createDepthTexture(AttachmentSpec const &spec) {
  auto const texture{createAndBindAttachmentTexture()};

  abcg::glTexImage2D(GL_TEXTURE_2D, 0, spec.internalFormat, m_size.x, m_size.y,
                     0, spec.format, spec.type, nullptr);

  abcg::glBindTexture(GL_TEXTURE_2D, 0);

  m_depthTexture = texture;

  abcg::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, texture, 0);
}

glm::ivec2 RenderTarget::getSize() const noexcept { return m_size; }

GLuint RenderTarget::getColorTexture(std::size_t index) const {
  if (index >= m_colorTextures.size()) {
    throw abcg::RuntimeError(
        std::format("Color attachment index {} out of range (max: {})", index,
                    m_colorTextures.size()));
  }
  return m_colorTextures[index];
}

GLuint RenderTarget::getDepthTexture() const noexcept { return m_depthTexture; }

std::size_t RenderTarget::getColorAttachmentCount() const noexcept {
  return m_colorTextures.size();
}
