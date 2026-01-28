/**
 * @file ui_legends.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "ui_legends.hpp"

#include "appcontext.hpp"
#include "renderstate.hpp"
#include "ui_widgets.hpp"

#include <cppitertools/itertools.hpp>
#include <gsl/gsl>

void uiLegends::isovalueLegendAndModeSettings(AppContext &context) {
  static constexpr auto kVerticalMargin{5.0f};
  static constexpr auto kHorizontalMargin{10.0f};
  static constexpr auto kMinWidth{246.0f};
  static constexpr auto kTooltip{"Sign relative to f(x,y,z) - isovalue"};

  auto &appState{context.appState};
  auto &renderState{context.renderState};

  ImGui::SetNextWindowPos(
      {gsl::narrow<float>(appState.windowSize.x) * 0.5f,
       gsl::narrow<float>(appState.windowSize.y) - 43.0f - kVerticalMargin},
      ImGuiCond_Always, {0.5f, 1.0f});

  ImGui::Begin("Legend & Mode AppContext", nullptr,
               ImGuiWindowFlags_NoDecoration |
                   ImGuiWindowFlags_AlwaysAutoResize);

  auto const availableWidth{gsl::narrow<float>(appState.windowSize.x) -
                            kHorizontalMargin};
  auto const wide{availableWidth >= kMinWidth};

  auto const *const labelPositive{wide ? "Positive side" : "+"};
  auto const *const labelNegative{wide ? "Negative side" : "-"};

  ImGui::ColorEdit3(labelPositive, &renderState.outsideKdId.x,
                    ImGuiColorEditFlags_NoInputs |
                        ImGuiColorEditFlags_NoTooltip);
  uiWidgets::showDelayedTooltip(kTooltip);

  ImGui::SameLine();
  ImGui::Dummy({0.0f, 5.0f});
  ImGui::SameLine();

  ImGui::ColorEdit3(labelNegative, &renderState.insideKdId.x,
                    ImGuiColorEditFlags_NoInputs |
                        ImGuiColorEditFlags_NoTooltip);
  uiWidgets::showDelayedTooltip(kTooltip);

  ImGui::End();
}

void uiLegends::dvrLegendAndModeSettings(AppContext &context) {
  static constexpr auto kVerticalMargin{5.0f};
  static constexpr auto kHorizontalMargin{5.0f};
  static constexpr auto kMaxWidth{800.0f};

  auto &appState{context.appState};
  auto &renderState{context.renderState};

  // Inverse of the sigmoid function used as the diverging colormap transfer
  // function
  auto const invTanh{[&k = renderState.dvrFalloff](float y) {
    return std::log(y / (1.0f - y)) / (2.0f * k);
  }};

  auto const width{
      std::min(kMaxWidth, gsl::narrow<float>(appState.windowSize.x) -
                              kHorizontalMargin * 2.0f)};
  auto const height{51.0f + 22.0f};
  auto const posY{gsl::narrow<float>(appState.windowSize.y) - 43.0f - height -
                  kVerticalMargin};
  ImGui::SetNextWindowPos(
      ImVec2((gsl::narrow<float>(appState.windowSize.x) - width) * 0.5f, posY));
  ImGui::SetNextWindowSize(ImVec2(width, height));
  ImGui::Begin("Legend & Mode AppContext", nullptr,
               ImGuiWindowFlags_NoDecoration);

  if (ImGui::BeginTable("##tblLegend", 2)) {
    auto const firstColumnWeight{0.75f};
    auto const secondColumnWeight{1.0f - firstColumnWeight};
    ImGui::TableSetupColumn("##tblLegendColumnA",
                            ImGuiTableColumnFlags_WidthStretch,
                            firstColumnWeight);
    ImGui::TableSetupColumn("##tblLegendColumnB",
                            ImGuiTableColumnFlags_WidthStretch,
                            secondColumnWeight);

    ImGui::TableNextColumn();

    auto const itemSpacingX{ImGui::GetStyle().ItemSpacing.x};
    auto const columnAWidth{(width * firstColumnWeight) -
                            (15.0f + itemSpacingX * 2.0f)};
    ImGui::Dummy({5.0f, 0.0f});
    ImGui::SameLine();
    uiWidgets::gradientWidget(
        "##gradientWidget", renderState.dvrColormap, true,
        ImVec2(columnAWidth, 53), invTanh, true,
        std::clamp(gsl::narrow_cast<int>(columnAWidth / 50.0f), 2, 50),
        "Negative", "Isovalue", "Positive");

    ImGui::TableNextColumn();

    ImGui::Dummy({2.0f, 0.0f});
    ImGui::SameLine();

    auto const columnBWidth{(width * secondColumnWeight) -
                            (itemSpacingX * 2.0f)};
    ImGui::PushItemWidth(columnBWidth);

    ImGui::SliderFloat("##sliderDvrDensity", &renderState.dvrDensity,
                       RenderState::kMinDvrDensity, RenderState::kMaxDvrDensity,
                       "Density: %.1f");

    renderState.dvrDensity =
        std::clamp(renderState.dvrDensity, 0.0f, RenderState::kMaxDvrDensity);

    ImGui::Spacing();

    ImGui::Dummy({2.0f, 0.0f});
    ImGui::SameLine();

    ImGui::BeginDisabled(appState.useRecommendedSettings);

    ImGui::SliderFloat("##sliderDvrFalloff", &renderState.dvrFalloff, 1e-5f,
                       50.0f, "Falloff: %.2g", ImGuiSliderFlags_Logarithmic);
    renderState.dvrFalloff = std::clamp(renderState.dvrFalloff, 1e-5f, 100.0f);

    ImGui::EndDisabled();

    uiWidgets::showRecommendedSettingsTooltip(context);

    ImGui::PopItemWidth();

    ImGui::EndTable();
  }

  ImGui::End();
}

void uiLegends::normalLegendAndModeSettings(AppContext &context) {
  static constexpr auto kVerticalMargin{5.0f};

  auto &appState{context.appState};
  auto &renderState{context.renderState};

  auto const itemSpacingX{ImGui::GetStyle().ItemSpacing.x};
  auto const unitNormal{renderState.surfaceColorMode ==
                        RenderState::SurfaceColorMode::UnitNormal};

  static constexpr std::array items{"Unit normal (XYZ to RGB)",
                                    "Normal magnitude"};
  static constexpr std::array itemsEnum{
      RenderState::SurfaceColorMode::UnitNormal,
      RenderState::SurfaceColorMode::NormalMagnitude};

  if (unitNormal) {
    static constexpr auto kHorizontalMargin{10.0f};
    static constexpr auto kCheckBoxWidth{122.0f};
    static constexpr auto kMaxComboWidth{kCheckBoxWidth + 200.0f};
    static constexpr auto kMinComboWidth{53.0f};

    ImGui::SetNextWindowPos(
        {gsl::narrow<float>(appState.windowSize.x) * 0.5f,
         gsl::narrow<float>(appState.windowSize.y) - 43.0f - kVerticalMargin},
        ImGuiCond_Always, {0.5f, 1.0f});

    ImGui::Begin("Legend & Mode AppContext", nullptr,
                 ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize);

    std::size_t const currentIndex{0};

    auto const width{std::min(
        kMaxComboWidth,
        gsl::narrow<float>(appState.windowSize.x) -
            (kHorizontalMargin * 2.0f + itemSpacingX + itemSpacingX / 2.0f))};
    ImGui::PushItemWidth(std::max(kMinComboWidth, width - kCheckBoxWidth));

    auto const newIndex{uiWidgets::combo("##comboNormal", items, currentIndex)};

    ImGui::PopItemWidth();

    ImGui::SameLine();

    if (newIndex != currentIndex) {
      auto const newMode{itemsEnum.at(newIndex)};
      renderState.surfaceColorMode = newMode;
    }

    auto inwardNormals{renderState.inwardNormals};
    ImGui::PushItemWidth(kCheckBoxWidth);
    ImGui::Checkbox("Inward normals", &inwardNormals);
    ImGui::PopItemWidth();
    if (inwardNormals != renderState.inwardNormals) {
      renderState.inwardNormals = inwardNormals;
    }
  } else {
    static constexpr auto kHorizontalMargin{5.0f};
    static constexpr auto kMaxWidth{800.0f};

    // Inverse of the sigmoid function used as the sequential colormap transfer
    // function
    auto &falloff{renderState.normalLengthFalloff};
    auto const invOneSidedTanh{[&falloff](float y) {
      return std::log((1.0f + y) / (1.0f - y)) / (2.0f * falloff);
    }};

    auto const width{
        std::min(kMaxWidth, gsl::narrow<float>(appState.windowSize.x) -
                                kHorizontalMargin * 2.0f)};
    auto const height{51.0f + 22.0f};
    auto const posY{gsl::narrow<float>(appState.windowSize.y) - 43.0f - height -
                    kVerticalMargin};
    ImGui::SetNextWindowPos(ImVec2(
        (gsl::narrow<float>(appState.windowSize.x) - width) * 0.5f, posY));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::Begin("Legend & Mode AppContext", nullptr,
                 ImGuiWindowFlags_NoDecoration);

    if (ImGui::BeginTable("##tblLegend", 2)) {
      auto const firstColumnWeight{0.75f};
      auto const secondColumnWeight{1.0f - firstColumnWeight};
      ImGui::TableSetupColumn("##tblLegendColumnA",
                              ImGuiTableColumnFlags_WidthStretch,
                              firstColumnWeight);
      ImGui::TableSetupColumn("##tblLegendColumnB",
                              ImGuiTableColumnFlags_WidthStretch,
                              secondColumnWeight);

      ImGui::TableNextColumn();

      auto const columnAWidth{(width * firstColumnWeight) -
                              (15.0f + itemSpacingX * 2.0f)};
      ImGui::Dummy({5.0f, 0.0f});
      ImGui::SameLine();

      uiWidgets::gradientWidget(
          "##gradientWidget", renderState.normalLengthColormap, false,
          ImVec2(columnAWidth, 53), invOneSidedTanh, true,
          std::clamp(gsl::narrow_cast<int>(columnAWidth / 50.0f), 2, 50), "",
          "Normal Magnitude", "");

      ImGui::TableNextColumn();

      ImGui::Dummy({2.0f, 0.0f});
      ImGui::SameLine();

      auto const columnBWidth{(width * secondColumnWeight) - itemSpacingX};

      ImGui::PushItemWidth(columnBWidth - itemSpacingX);

      auto const currentIndex{1};

      auto const newIndex{
          uiWidgets::combo("##comboNormal", items, currentIndex)};

      if (newIndex != currentIndex) {
        auto const newMode{itemsEnum.at(newIndex)};
        renderState.surfaceColorMode = newMode;
      }

      ImGui::Spacing();

      ImGui::Dummy({2.0f, 0.0f});
      ImGui::SameLine();

      ImGui::BeginDisabled(appState.useRecommendedSettings);

      ImGui::SliderFloat("##sliderNormalFalloff", &falloff, 1e-4f, 10.0f,
                         "Falloff: %.3g", ImGuiSliderFlags_Logarithmic);
      falloff = std::clamp(falloff, 1e-4f, 10.0f);

      ImGui::EndDisabled();

      uiWidgets::showRecommendedSettingsTooltip(context);

      ImGui::PopItemWidth();

      ImGui::EndTable();
    }
  }

  ImGui::End();
}

void uiLegends::curvatureLegendAndModeSettings(AppContext &context) {
  static constexpr auto kVerticalMargin{5.0f};
  static constexpr auto kHorizontalMargin{5.0f};
  static constexpr auto kMaxWidth{800.0f};

  auto &appState{context.appState};
  auto &renderState{context.renderState};

  auto const isGaussianCurvature{
      renderState.surfaceColorMode ==
      RenderState::SurfaceColorMode::GaussianCurvature};
  auto const isMeanCurvature{renderState.surfaceColorMode ==
                             RenderState::SurfaceColorMode::MeanCurvature};
  auto const isMaxAbsCurvature{renderState.surfaceColorMode ==
                               RenderState::SurfaceColorMode::MaxAbsCurvature};

  auto &falloff{[&]() -> float & {
    if (isGaussianCurvature) {
      return renderState.gaussianCurvatureFalloff;
    }
    if (isMeanCurvature) {
      return renderState.meanCurvatureFalloff;
    }
    return renderState.maxAbsCurvatureFalloff;
  }()};

  // Inverse of the sigmoid functions used as colormap transfer functions
  auto const invTanh{[&falloff](float y) {
    return std::log(y / (1.0f - y)) / (2.0f * falloff);
  }};
  auto const invOneSidedTanh{[&falloff](float y) {
    return std::log((1.0f + y) / (1.0f - y)) / (2.0f * falloff);
  }};

  auto const width{
      std::min(kMaxWidth, gsl::narrow<float>(appState.windowSize.x) -
                              kHorizontalMargin * 2.0f)};
  auto const height{51.0f + 22.0f};
  auto const posY{gsl::narrow<float>(appState.windowSize.y) - 43.0f - height -
                  kVerticalMargin};
  ImGui::SetNextWindowPos(
      ImVec2((gsl::narrow<float>(appState.windowSize.x) - width) * 0.5f, posY));
  ImGui::SetNextWindowSize(ImVec2(width, height));
  ImGui::Begin("Legend & Mode AppContext", nullptr,
               ImGuiWindowFlags_NoDecoration);

  if (ImGui::BeginTable("##tblLegend", 2)) {
    auto const firstColumnWeight{0.75f};
    auto const secondColumnWeight{1.0f - firstColumnWeight};
    ImGui::TableSetupColumn("##tblLegendColumnA",
                            ImGuiTableColumnFlags_WidthStretch,
                            firstColumnWeight);
    ImGui::TableSetupColumn("##tblLegendColumnB",
                            ImGuiTableColumnFlags_WidthStretch,
                            secondColumnWeight);

    ImGui::TableNextColumn();

    auto const itemSpacingX{ImGui::GetStyle().ItemSpacing.x};
    auto const columnAWidth{(width * firstColumnWeight) -
                            (15.0f + itemSpacingX * 2.0f)};
    ImGui::Dummy({5.0f, 0.0f});
    ImGui::SameLine();

    if (!isMaxAbsCurvature) {
      auto const *leftLabel{isGaussianCurvature ? "Hyperbolic" : "Concave*"};
      auto const *centerLabel{isGaussianCurvature ? "Parabolic" : "Minimal"};
      auto const *rightLabel{isGaussianCurvature ? "Elliptic" : "Convex*"};
      uiWidgets::gradientWidget(
          "##gradientWidget", renderState.curvatureColormap, false,
          ImVec2(columnAWidth, 53), invTanh, true,
          std::clamp(gsl::narrow_cast<int>(columnAWidth / 50.0f), 2, 50),
          leftLabel, centerLabel, rightLabel);
      if (renderState.surfaceColorMode ==
          RenderState::SurfaceColorMode::MeanCurvature) {
        uiWidgets::showDelayedTooltip(
            "* Relative to surface side\nConcave: "
            "Inward-bending\nConvex: Outward-bending");
      }
    } else {
      auto const *leftLabel{""};
      auto const *centerLabel{"Maximum Absolute Curvature"};
      auto const *rightLabel{""};
      uiWidgets::gradientWidget(
          "##gradientWidget", renderState.maxAbsCurvColormap, false,
          ImVec2(columnAWidth, 53), invOneSidedTanh, true,
          std::clamp(gsl::narrow_cast<int>(columnAWidth / 50.0f), 2, 50),
          leftLabel, centerLabel, rightLabel);
    }

    ImGui::TableNextColumn();

    ImGui::Dummy({2.0f, 0.0f});
    ImGui::SameLine();

    auto const columnBWidth{(width * secondColumnWeight) -
                            (itemSpacingX * 2.0f)};

    ImGui::PushItemWidth(columnBWidth);

    static constexpr std::array items{"Gaussian curvature (K)",
                                      "Mean curvature (H)", "max(|k1|, |k2|)"};
    static constexpr std::array itemsEnum{
        RenderState::SurfaceColorMode::GaussianCurvature,
        RenderState::SurfaceColorMode::MeanCurvature,
        RenderState::SurfaceColorMode::MaxAbsCurvature};

    Expects(renderState.surfaceColorMode ==
                RenderState::SurfaceColorMode::GaussianCurvature ||
            renderState.surfaceColorMode ==
                RenderState::SurfaceColorMode::MeanCurvature ||
            renderState.surfaceColorMode ==
                RenderState::SurfaceColorMode::MaxAbsCurvature);
    auto const currentIndex{[&]() -> std::size_t {
      if (renderState.surfaceColorMode ==
          RenderState::SurfaceColorMode::GaussianCurvature) {
        return 0;
      }
      if (renderState.surfaceColorMode ==
          RenderState::SurfaceColorMode::MeanCurvature) {
        return 1;
      }
      if (renderState.surfaceColorMode ==
          RenderState::SurfaceColorMode::MaxAbsCurvature) {
        return 2;
      }
      return 0;
    }()};

    auto const newIndex{
        uiWidgets::combo("##comboCurvature", items, currentIndex)};

    if (newIndex != currentIndex) {
      auto const newMode{itemsEnum.at(newIndex)};
      renderState.surfaceColorMode = newMode;
    }

    ImGui::Spacing();

    ImGui::Dummy({2.0f, 0.0f});
    ImGui::SameLine();

    ImGui::BeginDisabled(appState.useRecommendedSettings);

    ImGui::SliderFloat("##sliderCurvatureFalloff", &falloff, 1e-2f, 250.0f,
                       "Falloff: %.3g", ImGuiSliderFlags_Logarithmic);
    falloff = std::clamp(falloff, 1e-2f, 250.0f);

    ImGui::EndDisabled();

    uiWidgets::showRecommendedSettingsTooltip(context);

    ImGui::PopItemWidth();

    ImGui::EndTable();
  }

  ImGui::End();
}