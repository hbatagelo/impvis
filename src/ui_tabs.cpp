/**
 * @file ui_tabs.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImVis is released under the MIT license.
 */

#include "ui_tabs.hpp"

#include "appcontext.hpp"
#include "camera.hpp"
#include "functionmanager.hpp"
#include "raycast.hpp"
#include "renderstate.hpp"
#include "ui.hpp"
#include "ui_widgets.hpp"

#if defined(__EMSCRIPTEN__)
#include "ui_emscripten.hpp"
#endif

namespace {

constexpr std::string_view kAppVersion{"v3.0.0"};

} // namespace

void UITabs::functionsTab(AppContext &context, Camera &camera,
                          float parentWindowHeight) {
  auto &appState{context.appState};

  ImGui::BeginChild("##childFunctionsTab", ImVec2(0, parentWindowHeight - 109),
                    true, ImGuiWindowFlags_None);

  if (ImGui::IsWindowHovered()) {
    SDL_SetCursor(SDL_GetDefaultCursor());
  }

  auto itemOpened{false};

  for (auto &&[groupIndex, group] :
       iter::enumerate(context.functionManager.getGroups())) {
    ImGuiTreeNodeFlags const headerFlags{ImGuiTreeNodeFlags_CollapsingHeader};
    if (appState.updateFunctionTabSelection) {
      if (context.appState.selectedFunctionGroupIndex == groupIndex) {
        ImGui::SetNextItemOpen(true);
        appState.updateFunctionTabSelection = false;
        itemOpened = true;
      } else if (!itemOpened) {
        ImGui::SetNextItemOpen(false);
      }
    }
    if (ImGui::CollapsingHeader(group.name.c_str(), headerFlags)) {
      // functionHeader implementation
      if (ImGui::BeginTable(
              std::format("##tblFunctionHeader{}", groupIndex).c_str(), 2,
              ImGuiTableFlags_SizingFixedFit)) {

        ImVec2 const thumbSize{32, 32};
        for (auto &&[functionIndex, function] :
             iter::enumerate(group.functions)) {
          if (function.getData().name.empty()) {
            continue;
          }

          auto selected{appState.selectedFunctionGroupIndex == groupIndex &&
                        appState.selectedFunctionIndex == functionIndex};
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          if (auto const thumbnailID{
                  gsl::narrow<intptr_t>(function.getThumbnailId())};
              thumbnailID > 0) {
            auto const texture{gsl::narrow_cast<ImTextureID>(thumbnailID)};
            ImGui::Image(texture, thumbSize);
          }
          ImGui::TableNextColumn();
          auto const &data{function.getData()};
          ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,
                              ImVec2(0.0f, 0.45f));
          ImGui::Selectable(data.name.c_str(), &selected,
                            ImGuiSelectableFlags_SpanAllColumns,
                            ImVec2(0, thumbSize.y - 2));
          ImGui::PopStyleVar(1);
          if (selected && (appState.selectedFunctionGroupIndex != groupIndex ||
                           appState.selectedFunctionIndex != functionIndex)) {
            appState.useRecommendedSettings = true;
            appState.selectedFunctionGroupIndex = groupIndex;
            appState.selectedFunctionIndex = functionIndex;

            context.renderState.function = {context.functionManager.getGroups()
                                                .at(groupIndex)
                                                .functions.at(functionIndex)};

            auto const modelScale{
                data.scale * std::tan(glm::radians(camera.getFovY()) / 2.0f) /
                std::tan(glm::radians(30.0f) / 2.0f)};

            auto &renderState{context.renderState};
            auto const minScale{0.1f / renderState.boundsRadius};
            auto const maxScale{8.0f / renderState.boundsRadius};
            auto const clampedModelScale{
                glm::clamp(modelScale, minScale, maxScale)};

            camera.setModelScale(clampedModelScale);
#if defined(__EMSCRIPTEN__)
            emscriptenMathJax::updateEquationName(data.name);
            emscriptenMathJax::updateEquation(
                function.getMathJaxEquation(renderState.isoValue),
                appState.overlayMathJaxComment ? data.comment : "");
#endif
          }
        }
        ImGui::EndTable();
      }
    }
  }

  ImGui::EndChild();

  ImGui::Spacing();
  auto showFunctionEditor{context.appState.showFunctionEditor};
  ImGui::Checkbox("Show function editor", &showFunctionEditor);
  context.appState.showFunctionEditor = showFunctionEditor;
  context.appState.overlayMathJaxComment = !showFunctionEditor;

  static auto lastShowFunctionEditor{showFunctionEditor};
  if (lastShowFunctionEditor != showFunctionEditor) {
    lastShowFunctionEditor = showFunctionEditor;
#if defined(__EMSCRIPTEN__)
    auto const &data{context.renderState.function.getData()};
    emscriptenMathJax::updateEquation(
        context.renderState.function.getMathJaxEquation(
            context.renderState.isoValue),
        context.appState.overlayMathJaxComment ? data.comment : "");
#endif
  }
}

void UITabs::settingsTab(AppContext &context, Camera &camera) {
  ImGui::BeginChild("##childSettingsTab", ImVec2(0, -1),
                    ImGuiChildFlags_Borders);

  if (ImGui::IsWindowHovered()) {
    SDL_SetCursor(SDL_GetDefaultCursor());
  }

  auto &appState{context.appState};
  auto &renderState{context.renderState};

  ImGui::Checkbox("Use recommended settings", &appState.useRecommendedSettings);

  ImGui::SeparatorText("Bounding geometry");
  {
    ImGui::PushItemWidth(170);

    ImGui::BeginDisabled(appState.useRecommendedSettings);

    // Bounding geometry combo box
    static constexpr std::array<std::string_view, 2> items{"Box", "Sphere"};
    static constexpr std::array itemsEnum{RenderState::BoundsShape::Sphere,
                                          RenderState::BoundsShape::Box};

    auto const currentIndex{gsl::narrow<std::size_t>(renderState.boundsShape)};
    auto const newIndex{UIWidgets::combo("Shape", items, currentIndex)};
    auto const newBoundsShape{itemsEnum.at(newIndex)};
    renderState.boundsShape = newBoundsShape;

    // Bounding size (for box) or radius (for sphere)
    auto const label{renderState.boundsShape == RenderState::BoundsShape::Box
                         ? "Size"
                         : "Radius"};
    ImGui::SliderFloat(label, &renderState.boundsRadius, 0.1f, 20.0f, "%.1f");

    ImGui::EndDisabled();

    ImGui::PopItemWidth();
  }

  auto const DVRSelected{renderState.renderingMode ==
                         RenderState::RenderingMode::DirectVolume};

  ImGui::SeparatorText("Isosurface ray marching");
  {
    ImGui::PushItemWidth(156);

    // Disable isosurface raymarching options when using recommended settings or
    // direct volume rendering
    ImGui::BeginDisabled(appState.useRecommendedSettings || DVRSelected);

    // Raymarch method combo box
    static constexpr std::array<std::string_view, 2> items{"Adaptive",
                                                           "Fixed-step"};
    auto currentIndex{renderState.raymarchAdaptive ? 0ul : 1ul};
    auto const newIndex{UIWidgets::combo("Method", items, currentIndex)};
    renderState.raymarchAdaptive = newIndex == 0;

    // Isosurface raymarch steps
    auto const minSteps{5};
    auto const maxSteps{1500};
    ImGui::SliderInt("Steps##isosurface", &renderState.isosurfaceRaymarchSteps,
                     minSteps, maxSteps);

    // Root test combo box
    static constexpr std::array<std::string_view, 3> rootTestItems{
        "Sign change", "Taylor 1st-order", "Taylor 2nd-order"};
    static constexpr std::array rootTestItemsEnum{
        RenderState::RootTestMode::SignChange,
        RenderState::RootTestMode::Taylor1stOrder,
        RenderState::RootTestMode::Taylor2ndOrder};

    auto const currentRootIndex{
        gsl::narrow<std::size_t>(renderState.raymarchRootTest)};
    auto const newRootIndex{
        UIWidgets::combo("Root test", rootTestItems, currentRootIndex)};
    if (newRootIndex != currentRootIndex) {
      auto const newItem{rootTestItemsEnum.at(newRootIndex)};
      renderState.raymarchRootTest = newItem;
    }

    // Gradient evaluation combo box
    static constexpr std::array<std::string_view, 3> gradientItems{
        "Forward difference", "Central difference", "5-point stencil"};
    static constexpr std::array gradientItemsEnum{
        RenderState::GradientMode::ForwardDifference,
        RenderState::GradientMode::CentralDifference,
        RenderState::GradientMode::FivePointStencil};

    auto const currentGradientIndex{
        gsl::narrow<std::size_t>(renderState.raymarchGradientEvaluation)};
    auto const newGradientIndex{
        UIWidgets::combo("Gradient", gradientItems, currentGradientIndex)};
    if (newGradientIndex != currentGradientIndex) {
      auto const newItem{gradientItemsEnum.at(newGradientIndex)};
      renderState.raymarchGradientEvaluation = newItem;
    }

    ImGui::PopItemWidth();

    ImGui::EndDisabled();
  }

  ImGui::SeparatorText("Camera projection");
  {
    // Camera projection combo box
    static constexpr std::array<std::string_view, 2> camItems{"Perspective",
                                                              "Orthographic"};
    static constexpr std::array camItemsEnum{Camera::Projection::Perspective,
                                             Camera::Projection::Orthographic};

    auto const currentIndex{
        gsl::narrow<std::size_t>(appState.selectedCameraProjection)};

    ImGui::PushItemWidth(125);
    auto const newIndex{
        UIWidgets::combo("##comboProjection", camItems, currentIndex)};
    ImGui::PopItemWidth();

    if (newIndex != currentIndex) {
      auto const newProjection{camItemsEnum.at(newIndex)};
      appState.selectedCameraProjection = newProjection;
      camera.setProjection(newProjection);
    }

    // Disable FOV setting when projection is orthographic
    ImGui::BeginDisabled(appState.selectedCameraProjection ==
                         Camera::Orthographic);

    ImGui::SameLine(0.0f, 10.0f);
    ImGui::SetNextItemWidth(50.0f);
    auto fovY{appState.cameraFovY};
    if (ImGui::DragFloat("FOV", &fovY, 0.1f, 5.0f, 90.0f, "%.0f°")) {
      fovY = std::clamp(fovY, 5.0f, 90.0f);
      if (fovY != appState.cameraFovY) {
        auto const modelScale{camera.getModelScale() *
                              std::tan(glm::radians(fovY) / 2.0f) /
                              std::tan(glm::radians(camera.getFovY()) / 2.0f)};

        auto const minScale{0.1f / renderState.boundsRadius};
        auto const maxScale{10.0f / renderState.boundsRadius};
        auto const clampedModelScale{
            glm::clamp(modelScale, minScale, maxScale)};
        camera.setModelScale(clampedModelScale);

        appState.cameraFovY = fovY;
        camera.setFOV(fovY);
      }
    }
    if (ImGui::IsItemHovered() && !ImGui::IsAnyMouseDown()) {
      ImGui::SetTooltip("Drag to change");
    }

    ImGui::EndDisabled();
  }

  ImGui::SeparatorText("Rendering & UI");
  {
    // Shader combo box
    static constexpr std::array<std::string_view, 3> shaderItems{
        "Lit isosurface", "Unlit isosurface", "Volume rendering"};
    static constexpr std::array shaderItemsEnum{
        RenderState::RenderingMode::LitSurface,
        RenderState::RenderingMode::UnlitSurface,
        RenderState::RenderingMode::DirectVolume};

    auto const currentShaderIndex{
        gsl::narrow<std::size_t>(renderState.renderingMode)};

    ImGui::PushItemWidth(148);
    auto const newShaderIndex{
        UIWidgets::combo("Shader", shaderItems, currentShaderIndex)};
    ImGui::PopItemWidth();

    if (newShaderIndex != currentShaderIndex) {
      auto const newRenderingMode{shaderItemsEnum.at(newShaderIndex)};
      if (newRenderingMode == RenderState::RenderingMode::LitSurface) {
        // Enable shadows if surface is lit
        renderState.useShadows = true;
      }
      if (newRenderingMode == RenderState::RenderingMode::UnlitSurface) {
        // Disable shadows if surface is unlit
        renderState.useShadows = false;
      }
      renderState.renderingMode = newRenderingMode;
    }

    if (!DVRSelected) {
      // Surface color combo box
      static constexpr std::array<std::string_view, 6> colorItems{
          "Surface side (+/-)", "Unit normal",    "Normal magnitude",
          "Gaussian curvature", "Mean curvature", "max(|k1|, |k2|)"};
      static constexpr std::array colorItemsEnum{
          RenderState::SurfaceColorMode::SideSign,
          RenderState::SurfaceColorMode::UnitNormal,
          RenderState::SurfaceColorMode::NormalMagnitude,
          RenderState::SurfaceColorMode::GaussianCurvature,
          RenderState::SurfaceColorMode::MeanCurvature,
          RenderState::SurfaceColorMode::MaxAbsCurvature};

      auto const currentColorIndex{
          static_cast<std::size_t>(renderState.surfaceColorMode)};

      ImGui::PushItemWidth(148);
      auto const newColorIndex{
          UIWidgets::combo("Color code", colorItems, currentColorIndex)};
      ImGui::PopItemWidth();

      if (newColorIndex != currentColorIndex) {
        auto const newColorMode{colorItemsEnum.at(newColorIndex)};
        renderState.surfaceColorMode = newColorMode;
      }

      // Antialiasing combo box
      static constexpr std::array<std::string_view, 6> AAItems{
          "Off", "2x MSAA", "4x MSAA", "8x MSAA", "16x MSAA"};
      auto const currentAAIndex{
          gsl::narrow_cast<std::size_t>(std::log2(renderState.msaaSamples))};

      ImGui::PushItemWidth(148);
      auto const newAAIndex{
          UIWidgets::combo("Anti-alias", AAItems, currentAAIndex)};
      ImGui::PopItemWidth();

      renderState.msaaSamples = 1 << newAAIndex;

      ImGui::BeginDisabled(renderState.renderingMode !=
                           RenderState::RenderingMode::LitSurface);
      ImGui::Checkbox("Shadows", &renderState.useShadows);
      ImGui::EndDisabled();

      ImGui::SameLine(134.0f, 0.0f);
      ImGui::Checkbox("Fog", &renderState.useFog);
    } else {
      ImGui::BeginDisabled(appState.useRecommendedSettings);

      // DVR raymarch steps
      auto const minSteps{150};
      auto const maxSteps{1500};
      ImGui::SliderInt("Steps##dvr", &renderState.dvrRaymarchSteps, minSteps,
                       maxSteps);

      ImGui::EndDisabled();
    }

    ImGui::Checkbox("Background", &appState.drawBackground);
    ImGui::SameLine(134.0f, 0.0f);
    ImGui::Checkbox("Axes", &renderState.showAxes);

    ImGui::Checkbox("Info tooltip", &appState.showSurfaceInfoTooltip);

#if defined(__EMSCRIPTEN__)
    if (!UI::s_noEquation.has_value()) {
      auto showEquation{appState.showEquation};
      ImGui::SameLine(134.0f, 0.0f);
      ImGui::Checkbox("MathJax", &showEquation);
      if (showEquation != appState.showEquation) {
        emscriptenMathJax::setMathJaxVisibility(showEquation);
        appState.showEquation = showEquation;
      }
    }
#endif

    ImGui::Spacing();
    if (ImGui::Button("Hide UI windows", ImVec2(-1, 0))) {
      appState.showUI = false;
    }
    UIWidgets::showDelayedTooltip("Press any key to unhide");
  }

  ImGui::EndChild();
}

void UITabs::aboutTab([[maybe_unused]] AppContext &context,
                      Raycast const &raycast) {
  ImGui::BeginChild("##childAboutTab", ImVec2(0, -1), ImGuiChildFlags_Borders);

  if (ImGui::IsWindowHovered()) {
    SDL_SetCursor(SDL_GetDefaultCursor());
  }

  ImGui::Text("%s", std::format("ImpVis {}", kAppVersion).c_str());
  ImGui::TextUnformatted("3D Implicit Function Viewer");
  ImGui::TextUnformatted("Copyright (c) 2026 Harlen Batagelo");

  ImGui::SeparatorText("Statistics");
  {
    ImGui::PushItemWidth(168);

    auto const fpsUI{ImGui::GetIO().Framerate};
    auto const lastFrameTime{raycast.getLastFrameTime()};
    auto const fpsRender{1.0 / lastFrameTime};

    static std::size_t offsetUI{};
    static std::size_t offsetRender{};
    static auto refreshTime{ImGui::GetTime()};
    static std::array<float, 218> framesUI{};
    static std::array<float, 218> framesRender{};

    while (refreshTime < ImGui::GetTime()) {
      auto const refreshFrequency{60.0};

      framesUI.at(offsetUI) = fpsUI;
      offsetUI = (offsetUI + 1) % framesUI.size();

      framesRender.at(offsetRender) = gsl::narrow_cast<float>(fpsRender);
      offsetRender = (offsetRender + 1) % framesRender.size();

      refreshTime += (1.0 / refreshFrequency);
    }

    ImGui::PlotLines("##plotLinesFPSUI", framesUI.data(),
                     gsl::narrow<int>(framesUI.size()),
                     gsl::narrow<int>(offsetUI),
                     std::format("UI: {:.1f} FPS", fpsUI).c_str(), 0.0f,
                     *std::max_element(framesUI.begin(), framesUI.end()) * 2,
                     ImVec2(gsl::narrow<float>(framesUI.size()), 50));

    ImGui::PlotLines(
        "##plotLinesFPSRender", framesRender.data(),
        gsl::narrow<int>(framesRender.size()), gsl::narrow<int>(offsetRender),
        std::format("3D rendering: {:.1f} FPS", fpsRender).c_str(), 0.0f,
        *std::max_element(framesRender.begin(), framesRender.end()) * 2,
        ImVec2(gsl::narrow<float>(framesRender.size()), 50));
    ImGui::Spacing();

    ImGui::Text("%s",
                std::format("Render time: {:.2f} s", lastFrameTime).c_str());
    ImGui::Text(
        "%s",
        std::format("Render chunks: {}", raycast.getNumRenderChunks()).c_str());

    ImGui::PopItemWidth();
  }

#ifndef NDEBUG
  ImGui::SeparatorText("Developer settings");
  ImGui::Checkbox("Show debug info", &context.appState.showDebugInfo);
#if !defined(__EMSCRIPTEN__)
  ImGui::Spacing();
  if (ImGui::Button("Take screenshot", ImVec2(-1, 0))) {
    context.appState.takeScreenshot = true;
  }
#endif
#endif

  ImGui::EndChild();
}