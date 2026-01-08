/**
 * @file renderpipeline.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "renderpipeline.hpp"
#include "renderstate.hpp"

RenderPipeline::RenderPipeline()
    : m_arrow(), m_axes(), m_background(), m_raycast(), m_textureBlit() {}

void RenderPipeline::onCreate(RenderState const &renderState) {
  m_background.onCreate();
  m_raycast.onCreate(renderState);
  m_axes.onCreate();
  m_arrow.onCreate();
}

void RenderPipeline::onUpdate() { m_raycast.onUpdate(); }

void RenderPipeline::onPaint(RenderState &renderState, AppState const &appState,
                             Camera const &camera, glm::quat lightRotation) {
  if (appState.drawBackground && !appState.takeScreenshot) {
    auto const &backgroundTarget{m_backgroundTarget.getColorTexture()};
    m_background.onPaint(backgroundTarget);
    m_textureBlit.blit(backgroundTarget);
  } else {
    abcg::glClear(GL_COLOR_BUFFER_BIT);
  }

  m_axes.setCylinderLength(renderState.boundsRadius * 2.0f);

  auto onFrameStart{[&] {
    if (renderState.showAxes) {
      m_axesTarget.bind();
      abcg::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      abcg::glEnable(GL_DEPTH_TEST);
      abcg::glDepthMask(GL_TRUE);
      m_axes.setLightDirection(m_raycast.getLightDirection());
      m_axes.renderAxes(camera);
      abcg::glDisable(GL_DEPTH_TEST);

      m_raycastSwapChain.back().bind();
      m_raycast.setCompositionSource(m_axesTarget.getColorTexture(),
                                     m_axesTarget.getDepthTexture());
    }
  }};

  auto onFrameEnd{[&] {
    GLenum const drawBuffer{GL_COLOR_ATTACHMENT0};
    abcg::glDrawBuffers(1, &drawBuffer);

    if (renderState.surfaceColorMode ==
            RenderState::SurfaceColorMode::UnitNormal ||
        renderState.surfaceColorMode ==
            RenderState::SurfaceColorMode::NormalMagnitude) {
      m_arrow.setLightDirection(m_raycast.getLightDirection());
      m_arrow.render(camera);
    }

    if (renderState.showAxes) {
      glEnable(GL_DEPTH_TEST);
      m_axes.renderGlyphs(camera, renderState.boundsRadius,
                          renderState.renderingMode ==
                              RenderState::RenderingMode::DirectVolume);
      glDisable(GL_DEPTH_TEST);
      RenderTarget::unbind();
    }

    m_raycastSwapChain.swap();
  }};

  m_raycastSwapChain.back().bind();
  m_raycast.onPaint(camera, renderState, lightRotation, onFrameStart,
                    onFrameEnd);
  RenderTarget::unbind();

  abcg::glEnable(GL_BLEND);
  abcg::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  auto const t{std::clamp(ImGui::GetTime() / 1.5, 0.0, 1.0)};
  auto const fade{glm::smoothstep(0.0f, 1.0f, gsl::narrow_cast<float>(t))};
  m_textureBlit.blit(m_raycastSwapChain.front().getColorTexture(0),
                     glm::vec4{fade});
  abcg::glDisable(GL_BLEND);
}

void RenderPipeline::onResize(glm::ivec2 size) {
  m_axesTarget.resize(size);
  m_background.onResize(size);
  m_backgroundTarget.resize(size);
  m_raycastSwapChain.resize(size);
  m_raycast.onResize(size);
}

void RenderPipeline::onDestroy() {
  m_arrow.onDestroy();
  m_axes.onDestroy();
  m_raycast.onDestroy();
  m_background.onDestroy();
}

void RenderPipeline::setArrowState(bool visible, glm::vec3 position,
                                   glm::vec3 normal) noexcept {
  m_arrow.setVisible(visible);
  m_arrow.setPosition(position);
  m_arrow.setNormal(normal);
}

std::optional<RenderPipeline::PixelData>
RenderPipeline::readPixelData(glm::ivec2 pixelPosition) const {
  if (m_raycast.getFrameCount() == 0) {
    return std::nullopt;
  }

  m_raycastSwapChain.front().bind();

  abcg::glReadBuffer(GL_COLOR_ATTACHMENT1);
  glm::vec4 data{};

  abcg::glReadPixels(pixelPosition.x, pixelPosition.y, 1, 1, GL_RGBA, GL_FLOAT,
                     &data[0]);

  std::optional<PixelData> result;
  if (data.w > 0.5f) {
    PixelData pixelData{.position = glm::vec3(data)};

    if (m_raycastSwapChain.front().getColorAttachmentCount() > 2) {
      abcg::glReadBuffer(GL_COLOR_ATTACHMENT2);
      abcg::glReadPixels(pixelPosition.x, pixelPosition.y, 1, 1, GL_RGBA,
                         GL_FLOAT, &pixelData.extraData[0]);
    }
    result = pixelData;
  }

  RenderTarget::unbind();
  return result;
}