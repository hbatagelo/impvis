/**
 * @file window.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "window.hpp"
#include "renderstate.hpp"
#include "util.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

namespace emscriptenBindings {

void hideAxes() { Window::s_noAxes = std::monostate{}; }
void hideBackground() { Window::s_noBackground = std::monostate{}; }
void hideUI() { Window::s_noUI = std::monostate{}; }
void setInitialFunction(std::string name) {
  Window::s_initialFunctionName = name;
}

} // namespace emscriptenBindings

EMSCRIPTEN_BINDINGS(impvis_window) {
  emscripten::function("hideAxes", &emscriptenBindings::hideAxes);
  emscripten::function("hideBackground", &emscriptenBindings::hideBackground);
  emscripten::function("hideUI", &emscriptenBindings::hideUI);
  emscripten::function("setInitialFunction",
                       &emscriptenBindings::setInitialFunction);
}
#endif

void Window::onEvent(SDL_Event const &event) {
  glm::vec2 mousePosition;
  SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    m_trackBallLight.mouseMove(mousePosition);
  }
  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    if (event.button.button == SDL_BUTTON_LEFT) {
    }
    if (event.button.button == SDL_BUTTON_RIGHT) {
      m_trackBallLight.mousePress(mousePosition);
    }
  }
  if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
    if (event.button.button == SDL_BUTTON_LEFT) {
    }
    if (event.button.button == SDL_BUTTON_RIGHT) {
      m_trackBallLight.mouseRelease(mousePosition);
    }
  }
  if (event.type == SDL_EVENT_KEY_UP && !m_context.appState.showUI) {
#if defined(__EMSCRIPTEN__)
    if (!s_noUI.has_value())
#endif
    {
      m_context.appState.showUI = true;
    }
  }

  m_camera.handleEvent(event);
}

void Window::onCreate() {
  auto &appState{m_context.appState};
  auto &renderState{m_context.renderState};

  auto const &assetsPath{abcg::Application::getAssetsPath()};

  m_context.functionManager.loadFromDirectory(
      assetsPath / std::filesystem::path{"functions/"});

  selectInitialFunction();

  // Initialize render state from selected function
  auto const function{m_context.functionManager.getFunction(
      {.group = appState.selectedFunctionGroupIndex,
       .index = appState.selectedFunctionIndex})};

  if (function.has_value()) {
    renderState.function = function.value();
    renderState.boundsRadius = function->getData().boundsRadius;
  }

  if (appState.useRecommendedSettings && function) {
    m_camera.setModelScale(function->getData().scale);
  }

  m_pipeline.onCreate(renderState);
  m_ui.onCreate(m_context);

  abcg::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  abcg::glDisable(GL_DEPTH_TEST);
  abcg::glEnable(GL_CULL_FACE);

  // Blink caret in input text widgets
  ImGuiIO &guiIO{ImGui::GetIO()};
  guiIO.ConfigInputTextCursorBlink = true;
}

void Window::onUpdate() {
  m_camera.update();
  m_pipeline.onUpdate();
}

void Window::onPaint() {
  auto &appState{m_context.appState};
  auto &renderState{m_context.renderState};

  auto const viewportWidth{gsl::narrow<GLsizei>(appState.viewportSize.x)};
  auto const viewportHeight{gsl::narrow<GLsizei>(appState.viewportSize.y)};
  abcg::glViewport(0, 0, viewportWidth, viewportHeight);

  m_ui.onPaint();

  if (appState.useRecommendedSettings) {
    applyRecommendedSettings();
  }

  auto const minScale{0.1f / renderState.boundsRadius};
  auto const maxScale{8.0f / renderState.boundsRadius};
  auto const modelScale{m_camera.getModelScale()};
  auto const clampedModelScale{glm::clamp(modelScale, minScale, maxScale)};
  if (clampedModelScale != modelScale) {
    m_camera.setModelScale(clampedModelScale);
  }

  auto const lightRotation{m_trackBallLight.getRotation()};
  m_pipeline.onPaint(renderState, appState, m_camera, lightRotation);

  // Handle screenshots
  if (appState.takeScreenshot && m_pipeline.getRaycast().getFrameCount() > 0) {
    saveScreenshotPNG("screenshot.png");
    appState.takeScreenshot = false;
  }
}

void Window::onPaintUI() {
  abcg::OpenGLWindow::onPaintUI();

  auto const &appState{m_context.appState};

  // Disable UI when taking screenshots
  if (appState.takeScreenshot || !appState.showUI) {
    return;
  }

  m_ui.onPaintUI(m_context, m_pipeline, m_camera);
}

void Window::onResize([[maybe_unused]] glm::ivec2 size) {
  auto &appState{m_context.appState};

  auto *const window{getSDLWindow()};

  // Get the actual framebuffer size in pixels (HighDPI aware)
  int fbWidth{};
  int fbHeight{};
  SDL_GetWindowSizeInPixels(window, &fbWidth, &fbHeight);
  glm::ivec2 const fbSize{fbWidth, fbHeight};

  // Use framebuffer size for OpenGL viewport
  appState.viewportSize = fbSize;

  // Update ImGui with the correct scale
  ImGuiIO &guiIO{ImGui::GetIO()};

#if defined(__EMSCRIPTEN__)
  // For WASM: get DPR from JavaScript
  auto const dpr{gsl::narrow<float>(
      EM_ASM_DOUBLE({ return window.devicePixelRatio || 1.0; }))};
  appState.windowSize = {gsl::narrow<float>(size.x) / dpr,
                         gsl::narrow<float>(size.y) / dpr};
#else
  // For native: SDL handles HighDPI automatically
  int windowWidth{};
  int windowHeight{};
  SDL_GetWindowSize(window, &windowWidth, &windowHeight);
  appState.windowSize = {windowWidth, windowHeight};
  auto const dpr{SDL_GetWindowDisplayScale(window)};
#endif
  guiIO.DisplayFramebufferScale = ImVec2(dpr, dpr);

  appState.updateFunctionEditorLayout = true;
  appState.updateLogWindowLayout = true;

  m_pipeline.onResize(fbSize);
  m_camera.resize(size);
  m_trackBallLight.resizeViewport(size);
}

void Window::onDestroy() {
  m_ui.onDestroy();
  m_pipeline.onDestroy();
}

void Window::selectInitialFunction() {
  auto &appState{m_context.appState};

#if defined(__EMSCRIPTEN__)
  appState.showUI = !s_noUI.has_value();
  appState.drawBackground = !s_noBackground.has_value();
  m_context.renderState.showAxes = !s_noAxes.has_value();

  if (!s_initialFunctionName.empty()) {
    if (auto const id{m_context.functionManager.getId(s_initialFunctionName)}) {
      appState.selectedFunctionGroupIndex = id->group;
      appState.selectedFunctionIndex = id->index;
      return;
    } else {
      fmt::print(stderr, "Warning: Function '{}' not found, using default\n",
                 s_initialFunctionName);
      s_initialFunctionName.clear();
    }
  }
#endif

  // Use default selection if no match found
  auto const &groups{m_context.functionManager.getGroups()};
  if (appState.selectedFunctionGroupIndex < groups.size()) {
    auto const &functions{
        groups[appState.selectedFunctionGroupIndex].functions};
    if (appState.selectedFunctionIndex >= functions.size()) {
      appState.selectedFunctionIndex = 0;
    }
  }
}

void Window::applyRecommendedSettings() {
  auto const &data{m_context.renderState.function.getData()};
  auto &renderState{m_context.renderState};

  renderState.boundsShape = util::toLower(data.boundsShape) == "box"
                                ? RenderState::BoundsShape::Box
                                : RenderState::BoundsShape::Sphere;
  renderState.boundsRadius = data.boundsRadius;
  renderState.raymarchAdaptive =
      util::toLower(data.isosurfaceRaymarchMethod) != "fixed-step";

  auto const rootTestMode{util::toLower(data.isosurfaceRaymarchRootTest)};
  if (rootTestMode == "taylor 1st-order") {
    renderState.raymarchRootTest = RenderState::RootTestMode::Taylor1stOrder;
  } else if (rootTestMode == "taylor 2nd-order") {
    renderState.raymarchRootTest = RenderState::RootTestMode::Taylor2ndOrder;
  } else {
    renderState.raymarchRootTest = RenderState::RootTestMode::SignChange;
  }

  auto const gradientEvaluation{
      util::toLower(data.isosurfaceRaymarchGradientEvaluation)};
  if (gradientEvaluation == "central difference") {
    renderState.raymarchGradientEvaluation =
        RenderState::GradientMode::CentralDifference;
  } else if (gradientEvaluation == "5-point stencil") {
    renderState.raymarchGradientEvaluation =
        RenderState::GradientMode::FivePointStencil;
  } else {
    renderState.raymarchGradientEvaluation =
        RenderState::GradientMode::ForwardDifference;
  }

  renderState.dvrFalloff = data.dvrFalloff;
  renderState.gaussianCurvatureFalloff = data.gaussianCurvatureFalloff;
  renderState.meanCurvatureFalloff = data.meanCurvatureFalloff;
  renderState.maxAbsCurvatureFalloff = data.maxAbsCurvatureFalloff;
  renderState.normalLengthFalloff = data.normalLengthFalloff;

  auto rayMarchSteps{data.isosurfaceRaymarchSteps};

  // Force 5-point stencil and 2x step count for curvature visualization
  if (renderState.surfaceColorMode ==
          RenderState::SurfaceColorMode::GaussianCurvature ||
      renderState.surfaceColorMode ==
          RenderState::SurfaceColorMode::MeanCurvature ||
      renderState.surfaceColorMode ==
          RenderState::SurfaceColorMode::MaxAbsCurvature) {
    renderState.raymarchGradientEvaluation =
        RenderState::GradientMode::FivePointStencil;
    rayMarchSteps =
        gsl::narrow_cast<int>(gsl::narrow<float>(rayMarchSteps) * 2.0f);
  } else if (!renderState.useShadows && rayMarchSteps > 60) {
    rayMarchSteps =
        gsl::narrow_cast<int>(gsl::narrow<float>(rayMarchSteps) * 0.75f);
  }

  renderState.isosurfaceRaymarchSteps = rayMarchSteps;

  // Make number of raymarch steps proportional to density
  // If density = kInitialDvrDensity, use recommended number of steps
  // If density = kMaxDvrDensity, use triple the recommended number of steps
  renderState.dvrRaymarchSteps = gsl::narrow_cast<int>(
      gsl::narrow<float>(data.dvrRaymarchSteps) *
      std::lerp(
          1.0f, 3.0f,
          (renderState.dvrDensity - RenderState::kInitialDvrDensity) /
              (RenderState::kMaxDvrDensity - RenderState::kInitialDvrDensity)));
}
