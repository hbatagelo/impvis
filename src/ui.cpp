/**
 * @file ui.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "ui.hpp"

#include "appcontext.hpp"
#include "camera.hpp"
#include "raycast.hpp"
#include "renderstate.hpp"
#include "ui_editor.hpp"
#include "ui_legends.hpp"
#include "ui_tabs.hpp"
#include "ui_widgets.hpp"

#include <abcgUtil.hpp>

#if defined(__EMSCRIPTEN__)
#include "ui_emscripten.hpp"
#include <emscripten/bind.h>

namespace emscriptenBindings {

void hideEquation() { UI::s_noEquation = std::monostate{}; }

} // namespace emscriptenBindings

EMSCRIPTEN_BINDINGS(impvis_ui) {
  emscripten::function("hideEquation", &emscriptenBindings::hideEquation);
}

namespace {

EM_JS(void, jsUpdateEquation,
      (char const *equation, std::size_t equationLength, char const *comment,
       std::size_t commentLength),
      {
        updateEquation(UTF8ToString(equation, equationLength),
                       UTF8ToString(comment, commentLength));
      });

EM_JS(void, jsUpdateEquationName, (char const *name, std::size_t length),
      { updateEquationName(UTF8ToString(name, length)); });

EM_JS(void, jsSetMathJaxVisibility, (bool visible),
      { setMathJaxVisibility(visible); });

} // namespace

namespace emscriptenMathJax {

bool updateEquation(std::string const &equation, std::string const &comment) {
  jsUpdateEquation(equation.c_str(), equation.size(), comment.c_str(),
                   comment.size());
  return true;
}

bool updateEquationName(std::string const &name) {
  jsUpdateEquationName(name.c_str(), name.size());
  return true;
}

bool setMathJaxVisibility(bool visible) {
  jsSetMathJaxVisibility(visible);
  return true;
}

} // namespace emscriptenMathJax
#endif

namespace {

constexpr std::size_t kMainWindowWidth{251};

#ifndef NDEBUG
void debugInfo(AppContext &context, Camera &camera) {
  auto &appState{context.appState};

  if (appState.updateLogWindowLayout) {
    ImGui::SetNextWindowPos(
        {5.0f, gsl::narrow<float>(appState.windowSize.y) - 425.0f});
    ImGui::SetNextWindowSize(
        {gsl::narrow<float>(appState.windowSize.x) / 2.0f, 130});
    appState.updateLogWindowLayout = false;
  }

  ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
  {
    auto &renderState{context.renderState};

    ImGui::Text("%s",
                std::format("Model scale: {}", camera.getModelScale()).c_str());
    ImGui::Text(
        "%s",
        std::format(
            "Falloffs:\n  DVR: {:.3f}\n  Gaussian curvature: {:.3f}\n  Mean "
            "curvature: {:.3f}\n  Max. abs. curvature: {:.3f}\n  Normal "
            "length: {:.4f}\n",
            renderState.dvrFalloff, renderState.gaussianCurvatureFalloff,
            renderState.meanCurvatureFalloff,
            renderState.maxAbsCurvatureFalloff, renderState.normalLengthFalloff)
            .c_str());
    ImGui::Text("%s", std::format("DVR raymarch steps: {}\n",
                                  renderState.dvrRaymarchSteps)
                          .c_str());
    ImGui::Spacing();

    auto const &data{renderState.function.getData()};
    ImGui::Text("Original expression:\n%s", data.expression.c_str());
    ImGui::Spacing();
    ImGui::Text(
        "MathJax:\n%s",
        renderState.function.getMathJaxEquation(renderState.isoValue).c_str());
    ImGui::Spacing();
    ImGui::Text("GLSL:\n%s", renderState.function.getGLSLExpression().c_str());

    if (!renderState.function.getParameters().empty()) {
      std::string functionParams;
      for (auto const &functionParam : renderState.function.getParameters()) {
        functionParams += std::format("name: {} value: {}\n",
                                      functionParam.name, functionParam.value);
      }

      ImGui::Spacing();
      ImGui::Text("Parameters:\n%s", functionParams.c_str());
    }
  }
  ImGui::End();
}
#endif

} // namespace

void UI::onCreate(AppContext const &context) {
  updateEquation(context, true);

  auto const loadFont{[](std::string const &fontFile,
                         float fontSizePixels) -> ImFont * {
    auto const &assetsPath{abcg::Application::getAssetsPath()};
    ImFontConfig const fontConfig;
    ImGuiIO &guiIO{ImGui::GetIO()};
    auto *font{guiIO.Fonts->AddFontFromFileTTF(
        abcg::pathToUtf8(assetsPath / std::filesystem::path{fontFile}).c_str(),
        fontSizePixels, &fontConfig)};
    if (font == nullptr) {
      throw abcg::RuntimeError(std::format("Failed to load {}", fontFile));
    }
    return font;
  }};

  auto const proportionalFontSize{16.0f};
  constexpr auto proportionalFontFile{"fonts/Roboto-Medium.ttf"};
  m_proportionalFont = loadFont(proportionalFontFile, proportionalFontSize);

  auto const monospacedFontSize{18.0f};
  constexpr auto monospacedFontFile{"fonts/RobotoMono-Regular.ttf"};
  m_monospacedFont = loadFont(monospacedFontFile, monospacedFontSize);

  auto const smallFontSize{14.0f};
  m_smallFont = loadFont(proportionalFontFile, smallFontSize);

  auto const numTopButtons{4};
  auto const &assetsPath{abcg::Application::getAssetsPath()};
  for (auto const &index : iter::range(numTopButtons)) {
    auto const path{assetsPath / std::filesystem::path{std::format(
                                     "textures/top_button_{}.png", index)}};
    auto const textureID{abcg::loadOpenGLTexture(
        {.path = path, .generateMipmaps = true, .flipUpsideDown = false})};
    m_buttonTextures.push_back(textureID);
  }

  m_crossHairCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
}

void UI::onPaint() {
#if defined(__EMSCRIPTEN__)
  if (UI::s_noEquation.has_value()) {
    emscriptenMathJax::setMathJaxVisibility(false);
  }
#endif
}

void UI::onPaintUI(AppContext &context, RenderPipeline &pipeline,
                   Camera &camera) {
#if defined(__EMSCRIPTEN__)
  // Refresh equation rendering using MathJax
  static auto lastElapsedTime{0.0};
  if (auto const timeOut{0.125}; ImGui::GetTime() - lastElapsedTime > timeOut) {
    lastElapsedTime = ImGui::GetTime();
    static auto lastIsoValue{0.0f};
    if (lastIsoValue != context.renderState.isoValue) {
      updateEquation(context);
      lastIsoValue = context.renderState.isoValue;
    }
  }
#endif

  // Disable UI when taking screenshots
  if (context.appState.takeScreenshot || !context.appState.showUI) {
    return;
  }

  ImGui::PushFont(m_proportionalFont);

  mainWindow(context, camera, pipeline.getRaycast());

  switch (context.renderState.renderingMode) {
  case RenderState::RenderingMode::LitSurface:
  case RenderState::RenderingMode::UnlitSurface:
    switch (context.renderState.surfaceColorMode) {
    case RenderState::SurfaceColorMode::SideSign:
      uiLegends::isovalueLegendAndModeSettings(context);
      break;
    case RenderState::SurfaceColorMode::UnitNormal:
    case RenderState::SurfaceColorMode::NormalMagnitude:
      uiLegends::normalLegendAndModeSettings(context);
      break;
    case RenderState::SurfaceColorMode::GaussianCurvature:
    case RenderState::SurfaceColorMode::MeanCurvature:
    case RenderState::SurfaceColorMode::MaxAbsCurvature:
      uiLegends::curvatureLegendAndModeSettings(context);
      break;
    }
    break;
  case RenderState::RenderingMode::DirectVolume:
    uiLegends::dvrLegendAndModeSettings(context);
    break;
  }

  isoValueWindow(context);

  surfaceInfoTooltip(pipeline, context);

  ImGui::PopFont();
}

void UI::onDestroy() {
  for (auto &buttonTexture : m_buttonTextures) {
    abcg::glDeleteTextures(1, &buttonTexture);
  }
  SDL_DestroyCursor(m_crossHairCursor);
}

void UI::mainWindow(AppContext &context, Camera &camera,
                    Raycast const &raycast) {
  auto &appState{context.appState};
  auto &renderState{context.renderState};

  // Create main window widget
  auto const minUIWindowSize{ImVec2(kMainWindowWidth, 654)};
  auto const maxUIWindowSize{ImVec2(kMainWindowWidth, 716)};
  auto const legendWindowHeight{71};
  auto const isoValueWindowHeight{38};

  auto const &parameters{renderState.function.getParameters()};
  auto const parametersExtraHeight{
      !parameters.empty() ? 34 + (parameters.size() * 26) : 0ul};

  auto const parametersMarginBelow{parametersExtraHeight > 0 ? 4ul : 0ul};
  ImVec2 const uiWindowSize{
      minUIWindowSize.x,
      std::max(std::min(minUIWindowSize.y,
                        gsl::narrow<float>(appState.windowSize.y - 5) -
                            gsl::narrow<float>(5 + legendWindowHeight + 5 +
                                               isoValueWindowHeight + 5 +
                                               parametersExtraHeight +
                                               parametersMarginBelow)),
               std::min(maxUIWindowSize.y,
                        gsl::narrow<float>(appState.windowSize.y) * 0.5f))};

  ImGui::SetNextWindowPos(ImVec2(
      gsl::narrow<float>(appState.windowSize.x) - uiWindowSize.x - 5, 5));

  ImGui::SetNextWindowSize(uiWindowSize);

  ImGui::Begin("ImpVis", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

  if (ImGui::IsWindowHovered()) {
    SDL_SetCursor(SDL_GetDefaultCursor());
  }

  auto const isMainWindowCollapsed{ImGui::IsWindowCollapsed()};

  if (!isMainWindowCollapsed) {
    topButtonBar(context);

    ImGui::Spacing();

    if (ImGui::BeginTabBar("##tabMainWindow", ImGuiTabBarFlags_None)) {
      if (ImGui::BeginTabItem("Functions")) {
        uiTabs::functionsTab(context, camera, uiWindowSize.y - 63);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Settings")) {
        uiTabs::settingsTab(context, camera);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("About")) {
        uiTabs::aboutTab(context, raycast);
        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }
  ImGui::End();

  // Parameters
  auto const showParameters{!parameters.empty()};
  if (showParameters) {
    auto const height{(isMainWindowCollapsed ? 22 : uiWindowSize.y) + 10};
    ImGui::SetNextWindowPos(
        ImVec2(gsl::narrow<float>(appState.windowSize.x) - uiWindowSize.x - 5,
               height));
    ImGui::SetNextWindowSize(
        ImVec2(uiWindowSize.x, gsl::narrow<float>(parametersExtraHeight)));
    ImGui::Begin("Parameters", nullptr,
                 ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove);

    if (ImGui::IsWindowHovered()) {
      SDL_SetCursor(SDL_GetDefaultCursor());
    }

    auto const step{0.01f};
    auto const spacing{6};
    for (auto const &parameter : parameters) {
      auto value{parameter.value};

      // Left arrow button
      ImGui::PushButtonRepeat(true);
      if (ImGui::ArrowButton(
              std::format("##leftArrowParam{}", parameter.name).c_str(),
              ImGuiDir_Left)) {
        value -= step;
      }
      ImGui::PopButtonRepeat();

      // Drag slider
      ImGui::SameLine(0.0f, spacing);
      ImGui::PushItemWidth(178);
      auto const label{std::format("##dragParam{}", parameter.name)};
      auto const format{
          std::format("{}: {:.2f}", parameter.name, parameter.value)};
      ImGui::DragFloat(label.c_str(), &value, 0.01f, 0.f, 0.f, format.c_str(),
                       ImGuiSliderFlags_NoRoundToFormat);
      if (ImGui::IsItemHovered() && !ImGui::IsAnyMouseDown()) {
        ImGui::SetTooltip("Drag to change");
      }
      ImGui::PopItemWidth();

      // Right arrow button
      ImGui::SameLine(0.0f, spacing);
      ImGui::PushButtonRepeat(true);
      if (ImGui::ArrowButton(
              std::format("##rightArrowParam{}", parameter.name).c_str(),
              ImGuiDir_Right)) {
        value += step;
      }
      ImGui::PopButtonRepeat();

      renderState.function.setParameter(parameter.name, value);
    }

    ImGui::End();
  }

  progressIndicator(
      ImVec2(gsl::narrow<float>(appState.windowSize.x) - uiWindowSize.x - 5,
             (isMainWindowCollapsed ? 22 : uiWindowSize.y) + 10 +
                 gsl::narrow<float>(parametersExtraHeight) +
                 (showParameters ? 5.0f : 0.0f)),
      uiWindowSize.x, raycast);

#ifndef NDEBUG
  if (appState.showDebugInfo) {
    ImGui::PushFont(m_monospacedFont);
    debugInfo(context, camera);
    ImGui::PopFont();
  }
#endif

  if (appState.showFunctionEditor) {
    uiEditor::functionEditor(context, raycast, m_monospacedFont);
  }
}

void UI::topButtonBar(AppContext &context) {
  auto &renderState{context.renderState};

  struct ButtonInfo {
    char const *label;
    char const *tooltip;
    ImGuiKey shortcutKey;
  };

  static constexpr std::array<ButtonInfo, 4> buttonInfo{
      {{.label = "Shaded",
        .tooltip = "Shaded isosurface",
        .shortcutKey = ImGuiKey_1},
       {.label = "Volume",
        .tooltip = "Volume rendering\nof the scalar field",
        .shortcutKey = ImGuiKey_2},
       {.label = "Normals",
        .tooltip = "Isosurface colored\nby normals",
        .shortcutKey = ImGuiKey_3},
       {.label = "Curvature",
        .tooltip = "Isosurface colored\nby curvature",
        .shortcutKey = ImGuiKey_4}}};

  auto const &io{ImGui::GetIO()};
  auto const buttonSize{ImVec2{47.0f, 47.0f}};

  // Tighten horizontal spacing for the toolbar
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{5.0f, 4.0f});

  auto const activateMode{[&](std::size_t index) {
    switch (index) {
    case 0:
      renderState.renderingMode = RenderState::RenderingMode::LitSurface;
      renderState.surfaceColorMode = RenderState::SurfaceColorMode::SideSign;
      renderState.useFog = true;
      renderState.useShadows = true;
      break;
    case 1:
      renderState.renderingMode = RenderState::RenderingMode::DirectVolume;
      renderState.surfaceColorMode = RenderState::SurfaceColorMode::SideSign;
      renderState.useFog = false;
      renderState.useShadows = false;
      break;
    case 2:
      renderState.renderingMode = RenderState::RenderingMode::UnlitSurface;
      renderState.surfaceColorMode = RenderState::SurfaceColorMode::UnitNormal;
      renderState.useFog = false;
      renderState.useShadows = false;
      break;
    case 3:
      renderState.renderingMode = RenderState::RenderingMode::UnlitSurface;
      renderState.surfaceColorMode =
          RenderState::SurfaceColorMode::GaussianCurvature;
      renderState.useFog = false;
      renderState.useShadows = false;
      break;
    default:
      break;
    }
  }};

  // Keyboard shortcuts: Ctrl + [1–4]
  if (io.KeyCtrl) {
    for (auto const index : iter::range(buttonInfo.size())) {
      if (ImGui::IsKeyPressed(buttonInfo.at(index).shortcutKey, false)) {
        activateMode(index);
      }
    }
  }

  for (auto const index : iter::range(m_buttonTextures.size())) {
    auto const groupStartX{ImGui::GetCursorPosX()};
    ImGui::BeginGroup();

    // Image button
    RenderState::RenderingMode expectedRenderingMode{};
    if (index == 1) {
      expectedRenderingMode = RenderState::RenderingMode::DirectVolume;
    } else if (index == 0) {
      expectedRenderingMode = RenderState::RenderingMode::LitSurface;
    } else {
      expectedRenderingMode = RenderState::RenderingMode::UnlitSurface;
    }

    RenderState::SurfaceColorMode expectedSurfaceColorMode{};
    if (index == 0 || index == 1) {
      expectedSurfaceColorMode = RenderState::SurfaceColorMode::SideSign;
    } else if (index == 2) {
      expectedSurfaceColorMode = RenderState::SurfaceColorMode::UnitNormal;
    } else {
      expectedSurfaceColorMode =
          RenderState::SurfaceColorMode::GaussianCurvature;
    }

    auto const selected{renderState.renderingMode == expectedRenderingMode &&
                        renderState.surfaceColorMode ==
                            expectedSurfaceColorMode};

    if (selected) {
      auto const colorSelected{ImVec4{0.62f, 0.62f, 0.62f, 1.0f}};
      ImGui::PushStyleColor(ImGuiCol_Button, colorSelected);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorSelected);
    } else {
      auto const color{ImVec4{0.11f, 0.11f, 0.11f, 1.0f}};
      ImGui::PushStyleColor(ImGuiCol_Button, color);
    }

    auto const texture{gsl::narrow<ImTextureID>(
        gsl::narrow<intptr_t>(m_buttonTextures.at(index)))};

    ImGui::PushID(gsl::narrow<int>(index));
    if (ImGui::ImageButton("##imgTopButton", texture, buttonSize)) {
      activateMode(index);
    }
    ImGui::PopID();

    auto const tooltip{std::format("{}\nShortcut: Ctrl+{}",
                                   buttonInfo.at(index).tooltip, index + 1)};
    uiWidgets::showDelayedTooltip(tooltip.c_str());

    if (selected) {
      ImGui::PopStyleColor(2);
    } else {
      ImGui::PopStyleColor();
    }

    // Label
    ImGui::SetWindowFontScale(0.8f);

    auto const *const label{buttonInfo.at(index).label};
    auto const labelSize{ImGui::CalcTextSize(label)};
    auto const &style{ImGui::GetStyle()};
    auto const insideMargin{style.FramePadding.x * 2.0f};

    auto const labelX{groupStartX +
                      (((buttonSize.x + insideMargin) - labelSize.x) * 0.5f)};

    ImGui::SetCursorPosX(labelX);
    ImGui::TextUnformatted(label);

    ImGui::SetWindowFontScale(1.0f);

    ImGui::EndGroup();
    ImGui::SameLine();
  }

  ImGui::PopStyleVar();
  ImGui::Spacing();
}

void UI::isoValueWindow(AppContext &context) {
  static constexpr auto kVerticalMargin{5.0f};
  static constexpr auto kHorizontalMargin{5.0f};
  static constexpr auto kDragWidth{50.0f};
  static constexpr auto kResetButtonWidth{50.0f};
  static constexpr auto kMaxWidth{800.0f};

  auto &appState{context.appState};
  auto &renderState{context.renderState};

  auto const width{
      std::min(kMaxWidth, gsl::narrow<float>(appState.windowSize.x) -
                              kHorizontalMargin * 2.0f)};
  ImGui::Begin("Isovalue", nullptr,
               ImGuiWindowFlags_NoDecoration |
                   ImGuiWindowFlags_AlwaysAutoResize);

  static constexpr auto defaultIsoMin{-2.0f};
  static constexpr auto defaultIsoMax{2.0f};
  static auto isoMin{defaultIsoMin};
  static auto isoMax{defaultIsoMax};
  renderState.isoValue = std::clamp(renderState.isoValue, isoMin, isoMax);

  // Minimum iso value
  ImGui::PushItemWidth(kDragWidth);
  ImGui::DragFloat("##dragIsovalueMin", &isoMin, 0.1f, -1e5f, -0.1f, "%.1g");
  if (ImGui::IsItemHovered() && !ImGui::IsAnyMouseDown()) {
    ImGui::SetTooltip("Drag to change\nminimum value");
  }
  ImGui::PopItemWidth();

  // Slider expands horizontally
  ImGui::SameLine();
  auto const itemSpacingX{ImGui::GetStyle().ItemSpacing.x};
  auto const sliderMargin{itemSpacingX + kDragWidth + itemSpacingX +
                          itemSpacingX + kDragWidth + itemSpacingX +
                          kResetButtonWidth};
  ImGui::PushItemWidth(width - sliderMargin);
  ImGui::SliderFloat("##sliderIsovalue", &renderState.isoValue, isoMin, isoMax,
                     "Isovalue: %.3g", ImGuiSliderFlags_NoRoundToFormat);
  if (ImGui::IsItemHovered() && !ImGui::IsAnyMouseDown()) {
    ImGui::SetTooltip("Ctrl+click to\ninput value");
  }
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Maximum iso value
  ImGui::PushItemWidth(kDragWidth);
  ImGui::DragFloat("##dragIsovalueMax", &isoMax, 0.1f, 0.1f, 1e5f, "%.1g");
  if (ImGui::IsItemHovered() && !ImGui::IsAnyMouseDown()) {
    ImGui::SetTooltip("Drag to change\nmaximum value");
  }
  ImGui::PopItemWidth();

  // Reset button
  ImGui::SameLine();
  ImGui::PushItemWidth(kResetButtonWidth);
  if (ImGui::Button("Reset")) {
    isoMin = defaultIsoMin;
    isoMax = defaultIsoMax;
    renderState.isoValue = 0.0f;
  }
  ImGui::PopItemWidth();
  ImGui::SameLine();

  auto const widgetSize{ImGui::GetWindowSize()};
  ImGui::SetWindowPos(
      ImVec2((gsl::narrow<float>(appState.windowSize.x) - widgetSize.x) * 0.5f,
             gsl::narrow<float>(appState.windowSize.y) - widgetSize.y -
                 kVerticalMargin));

  ImGui::End();
}

void UI::progressIndicator(ImVec2 position, float width,
                           Raycast const &raycast) {
  // Only show when rendering is slow
  if (!raycast.isFrameComplete() && raycast.getNumRenderChunks() > 30) {
    auto const progress{raycast.getRenderProgress()};

    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(ImVec2(width, 0));

    ImGui::Begin("Rendering Progress", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav);

    ImGui::TextUnformatted("Rendering...");
    ImGui::ProgressBar(progress, ImVec2(-1, 0), nullptr);

    ImGui::End();
  }
}

void UI::updateEquation([[maybe_unused]] AppContext const &context,
                        [[maybe_unused]] bool includeName) {
#if defined(__EMSCRIPTEN__)
  auto &appState{context.appState};
  auto &renderState{context.renderState};

  auto const &data{renderState.function.getData()};
  if (includeName) {
    emscriptenMathJax::updateEquationName(data.name);
  }
  emscriptenMathJax::updateEquation(
      renderState.function.getMathJaxEquation(renderState.isoValue),
      appState.overlayMathJaxComment ? data.comment : "");
#endif
}

void UI::surfaceInfoTooltip(RenderPipeline &pipeline,
                            AppContext const &context) {
  auto const &appState{context.appState};
  auto const &renderState{context.renderState};

  if (!appState.showSurfaceInfoTooltip) {
    return;
  }

  ImGuiIO &guiIO{ImGui::GetIO()};

  // Must be hovering over viewport (not over UI)
  if (!guiIO.WantCaptureMouse) {
    if (appState.showSurfaceInfoTooltip ||
        renderState.surfaceColorMode ==
            RenderState::SurfaceColorMode::UnitNormal ||
        renderState.surfaceColorMode ==
            RenderState::SurfaceColorMode::NormalMagnitude) {
      glm::ivec2 mousePosition{guiIO.MousePos.x, guiIO.MousePos.y};
      auto dpr{appState.viewportSize.x /
               gsl::narrow<float>(appState.windowSize.x)};

      glm::ivec2 const pixelPosition{
          gsl::narrow_cast<int>(gsl::narrow<float>(mousePosition.x) * dpr),
          gsl::narrow_cast<int>(appState.viewportSize.y -
                                (gsl::narrow<float>(mousePosition.y) * dpr)) -
              1};

      m_lastPixelData = pipeline.readPixelData(pixelPosition);
      if (m_lastPixelData.has_value()) {
        SDL_SetCursor(m_crossHairCursor);
        pipeline.setArrowState(true, m_lastPixelData->position,
                               m_lastPixelData->extraData);
      } else {
        SDL_SetCursor(SDL_GetDefaultCursor());
        pipeline.setArrowState(false, {}, {});
      }
    }
  } else {
    m_lastPixelData = std::nullopt;
    pipeline.setArrowState(false, {}, {});
  }

  auto const pixelData{m_lastPixelData};

  if (!pixelData.has_value()) {
    return;
  }

  ImGui::PushFont(m_smallFont);
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.75f);
  ImGui::BeginTooltip();

  auto const showText{
      []<typename... Args>(std::format_string<Args...> fmt, Args &&...args) {
        auto const text{std::format(fmt, std::forward<Args>(args)...)};
        ImGui::TextUnformatted(text.c_str());
      }};

  auto const surfaceMode{renderState.surfaceColorMode};

  if (renderState.renderingMode != RenderState::RenderingMode::DirectVolume) {
    showText("Position: ({:.2g}, {:.2g}, {:.2g})", pixelData->position.x,
             pixelData->position.y, pixelData->position.z);
    if (surfaceMode == RenderState::SurfaceColorMode::UnitNormal ||
        surfaceMode == RenderState::SurfaceColorMode::NormalMagnitude) {
      showText("Unit normal: ({:.2g}, {:.2g}, {:.2g})", pixelData->extraData.x,
               pixelData->extraData.y, pixelData->extraData.z);
      showText("Normal magnitude: {:.2g}", pixelData->extraData.w);
    } else if (surfaceMode ==
                   RenderState::SurfaceColorMode::GaussianCurvature ||
               surfaceMode == RenderState::SurfaceColorMode::MeanCurvature ||
               surfaceMode == RenderState::SurfaceColorMode::MaxAbsCurvature) {
      showText("Gaussian (K): {:.2g}", pixelData->extraData.x);
      showText("Mean (H): {:.2g}", pixelData->extraData.y);
      showText("Principal (k1): {:.2g}", pixelData->extraData.z);
      showText("Principal (k2): {:.2g}", pixelData->extraData.w);
    }
  } else {
    showText("Max contributing position:\n({:.2g}, {:.2g}, {:.2g})",
             pixelData->position.x, pixelData->position.y,
             pixelData->position.z);
    ImGui::Separator();
    showText("Optical depth: {:.2g}", pixelData->extraData.x);
    showText("Avg. scalar (weigthed): {:.2g}", pixelData->extraData.y);
    showText("Opacity: {:.3g}%", pixelData->extraData.z * 100.0f);
  }

  ImGui::EndTooltip();
  ImGui::PopStyleVar();
  ImGui::PopFont();
}