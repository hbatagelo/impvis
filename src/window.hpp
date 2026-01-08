/**
 * @file window.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef WINDOW_HPP_
#define WINDOW_HPP_

#include "appcontext.hpp"
#include "camera.hpp"
#include "renderpipeline.hpp"
#include "ui.hpp"

#include <imgui.h>

class Window : public abcg::OpenGLWindow {
protected:
  void onEvent(SDL_Event const &event) override;
  void onCreate() override;
  void onUpdate() override;
  void onPaint() override;
  void onPaintUI() override;
  void onResize(glm::ivec2 size) override;
  void onDestroy() override;

private:
  AppContext m_context{};
  RenderPipeline m_pipeline;
  Camera m_camera;
  UI m_ui;

  abcg::TrackBall m_trackBallLight;

  void applyRecommendedSettings();
  void selectInitialFunction();

public:
#if defined(__EMSCRIPTEN__)
  static inline std::string s_initialFunctionName;
  static inline std::optional<std::monostate> s_noAxes;
  static inline std::optional<std::monostate> s_noBackground;
  static inline std::optional<std::monostate> s_noUI;
#endif

  friend class UI;
};

#endif
