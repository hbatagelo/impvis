/**
 * @file renderpipeline.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef RENDERPIPELINE_HPP_
#define RENDERPIPELINE_HPP_

#include "appstate.hpp"
#include "arrow.hpp"
#include "axes.hpp"
#include "background.hpp"
#include "raycast.hpp"
#include "renderstate.hpp"
#include "rendertarget.hpp"
#include "swapchain.hpp"
#include "textureblit.hpp"

class RenderPipeline {
public:
  RenderPipeline();

  void onCreate(RenderState const &renderState);
  void onUpdate();
  void onPaint(RenderState &renderState, AppState const &appState,
               Camera const &camera, glm::quat lightRotation);
  void onResize(glm::ivec2 size);
  void onDestroy();

  void setArrowState(bool visible, glm::vec3 position,
                     glm::vec3 normal) noexcept;

  [[nodiscard]] Raycast const &getRaycast() const noexcept { return m_raycast; }
  [[nodiscard]] glm::vec3 getLightDirection() const noexcept {
    return m_raycast.getLightDirection();
  }

  struct PixelData {
    glm::vec3 position{};
    glm::vec4 extraData{};
  };
  [[nodiscard]] std::optional<PixelData>
  readPixelData(glm::ivec2 pixelPosition) const;

private:
  RenderTarget m_axesTarget{{
      RenderTarget::kRGBA8,   // Color
      RenderTarget::kDepth24, // Depth
  }};
  RenderTarget m_backgroundTarget{
      {RenderTarget::kRGBA8}, // Color
  };
  SwapChain m_raycastSwapChain{{
      RenderTarget::kRGBA8,   // Color
      RenderTarget::kDepth24, // Depth
      RenderTarget::kRGBA32F, // Data #0
      RenderTarget::kRGBA32F, // Data #1
  }};

  Arrow m_arrow;
  Axes m_axes;
  Background m_background;
  Raycast m_raycast;
  TextureBlit m_textureBlit;
};

#endif