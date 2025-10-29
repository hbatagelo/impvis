/**
 * @file settings.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include "equation.hpp"

struct Settings {
  Equation equation;
  float isoValue{};
  float colormapScale{0.75f};

  std::size_t selectedUIGroupIndex{};
  std::size_t selectedUIEquationIndex{};

  // Settings that when changed require rebuilding the shader program
  struct RenderSettings {
    bool useBoundingBox{};
    float boundRadius{2.5f};

    bool rayMarchAdaptive{true};
    int rayMarchSteps{150};
    bool rayMarchSignTest{true};

    std::size_t shaderIndex{};
    bool useShadows{true};
    bool useFog{true};
    bool useNormalsAsColors{};

    friend bool operator==(RenderSettings const &,
                           RenderSettings const &) = default;
  } renderSettings{};

  bool usePredefinedSettings{true};
  bool showEquationEditor{};
  bool showParserDebugInfo{};
  bool drawBackground{true};
  bool drawUI{true};
  bool overlayMathJaxComment{true};

  glm::vec2 viewportSize{};
  glm::ivec2 windowSize{};
  bool redrawBackgroundRenderTex{true};
  bool updateEquationEditorLayout{true};
  bool updateLogWindowLayout{true};
  bool takeScreenshot{};

  bool rebuildProgram{};
};

#endif
