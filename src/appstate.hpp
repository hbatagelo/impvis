/**
 * @file appstate.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef APPSTATE_HPP_
#define APPSTATE_HPP_

#include "camera.hpp"

#include <glm/glm.hpp>

struct AppState {
  bool showUI{true};
  bool showFunctionEditor{false};
  bool showSurfaceInfoTooltip{true};
  bool drawBackground{true};
  bool overlayMathJaxComment{true};
  bool useRecommendedSettings{true};

#ifndef NDEBUG
  bool showDebugInfo{false};
#endif

#if defined(__EMSCRIPTEN__)
  bool showEquation{true};
#endif

  glm::vec2 viewportSize{};
  glm::ivec2 windowSize{};

  float cameraFovY{30.0f};

  std::size_t selectedFunctionGroupIndex{1}; // Cubic
  std::size_t selectedFunctionIndex{0};      // Cayley
  Camera::Projection selectedCameraProjection{Camera::Perspective};

  bool updateFunctionEditorLayout{true};
  bool updateFunctionTabSelection{true};
  bool updateLogWindowLayout{true};
  bool takeScreenshot{false};
};

#endif
