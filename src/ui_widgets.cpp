/**
 * @file ui_widgets.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "ui_widgets.hpp"

#include "appcontext.hpp"

#include <cppitertools/itertools.hpp>
#include <format>
#include <gsl/gsl>
#include <imgui_internal.h>

void uiWidgets::showDelayedTooltip(char const *text, bool allowWhenDisabled) {
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal |
                           (allowWhenDisabled
                                ? ImGuiHoveredFlags_AllowWhenDisabled
                                : ImGuiHoveredFlags_None))) {
    ImGui::SetTooltip("%s", text);
  }
}

void uiWidgets::showRecommendedSettingsTooltip(AppContext &context) {
  if (context.appState.useRecommendedSettings) {
    showDelayedTooltip("Overridden by recommended settings", true);
  }
}

void uiWidgets::drawCheckerboard(ImDrawList *drawList, ImVec2 pos, ImVec2 size,
                                 float checkerSize) {
  auto const color1{IM_COL32(204, 204, 204, 255)};
  auto const color2{IM_COL32(153, 153, 153, 255)};

  auto const cols{gsl::narrow_cast<int>(std::ceil(size.x / checkerSize))};
  auto const rows{gsl::narrow_cast<int>(std::ceil(size.y / checkerSize))};

  for (auto const [row, col] :
       iter::product(iter::range(rows), iter::range(cols))) {
    auto const color{((row + col) % 2 == 0) ? color1 : color2};

    auto const minCorner{
        ImVec2(pos.x + (gsl::narrow<float>(col) * checkerSize),
               pos.y + (gsl::narrow<float>(row) * checkerSize))};
    auto const maxCorner{
        ImVec2(std::min(pos.x + gsl::narrow<float>(col + 1) * checkerSize,
                        pos.x + size.x),
               std::min(pos.y + gsl::narrow<float>(row + 1) * checkerSize,
                        pos.y + size.y))};

    drawList->AddRectFilled(minCorner, maxCorner, color);
  }
}

namespace {

struct GradientDimensions {
  float labelHeight;
  float tickerHeight;
  float barHeight;
  float markerSize;
};

struct GradientPositions {
  ImVec2 labelPos;
  ImVec2 barPos;
  ImVec2 barSize;
};

ImU32 colorToImU32(glm::vec4 color) {
  return IM_COL32(gsl::narrow_cast<int>(color.r * 255.0f),
                  gsl::narrow_cast<int>(color.g * 255.0f),
                  gsl::narrow_cast<int>(color.b * 255.0f),
                  gsl::narrow_cast<int>(color.a * 255.0f));
}

glm::vec4 interpolateColor(std::vector<glm::vec4> const &stops, float t) {
  if (stops.size() == 1) {
    return stops[0];
  }

  auto const tStop{t * gsl::narrow<float>(stops.size() - 1)};
  auto const idx{std::clamp(gsl::narrow<std::size_t>(std::floor(tStop)),
                            std::size_t{}, stops.size() - 2)};

  auto const tLocal{tStop - gsl::narrow<float>(idx)};
  return glm::mix(stops.at(idx), stops.at(idx + 1), tLocal);
}

GradientDimensions calculateDimensions(ImVec2 size, bool hasLabels,
                                       bool showTickers) {
  auto const smallFontSize{ImGui::GetFontSize() * 0.95f};
  auto const labelHeight{hasLabels ? smallFontSize + 4.0f : 0.0f};
  auto constexpr tickerMarkHeight{4.0f};
  auto const tickerHeight{showTickers ? tickerMarkHeight + smallFontSize + 3.0f
                                      : 0.0f};
  auto const barHeight{size.y - labelHeight - tickerHeight};

  return {.labelHeight = labelHeight,
          .tickerHeight = tickerHeight,
          .barHeight = barHeight,
          .markerSize = 10.0f};
}

GradientPositions calculatePositions(ImVec2 pos, ImVec2 size,
                                     GradientDimensions const &dims) {
  auto const labelPos{ImVec2(pos.x, pos.y)};
  auto const barPos{ImVec2(pos.x, pos.y + dims.labelHeight)};
  auto const barSize{ImVec2(size.x, dims.barHeight)};

  return {.labelPos = labelPos, .barPos = barPos, .barSize = barSize};
}

void drawGradientLabels(ImDrawList *drawList, ImVec2 labelPos, ImVec2 size,
                        char const *leftLabel, char const *centerLabel,
                        char const *rightLabel, float labelHeight) {
  if (labelHeight <= 0.0f) {
    return;
  }

  auto *const font{ImGui::GetFont()};
  auto const smallFontSize{ImGui::GetFontSize() * 0.95f};
  auto const offsetPosY{labelPos.y - 1};
  auto const labelColor{IM_COL32(200, 200, 200, 255)};

  if (*leftLabel != '\0') {
    drawList->AddText(font, smallFontSize, ImVec2(labelPos.x, offsetPosY),
                      labelColor, leftLabel);
  }

  if (*centerLabel != '\0') {
    auto const textSize{
        font->CalcTextSizeA(smallFontSize, FLT_MAX, 0.0f, centerLabel)};
    auto const centerPos{
        ImVec2(labelPos.x + ((size.x - textSize.x) * 0.5f), offsetPosY)};
    drawList->AddText(font, smallFontSize, centerPos, labelColor, centerLabel);
  }

  if (*rightLabel != '\0') {
    auto const textSize{
        font->CalcTextSizeA(smallFontSize, FLT_MAX, 0.0f, rightLabel)};
    auto const rightPos{ImVec2(labelPos.x + size.x - textSize.x, offsetPosY)};
    drawList->AddText(font, smallFontSize, rightPos, labelColor, rightLabel);
  }
}

void drawGradientBar(ImDrawList *drawList, ImVec2 barPos, ImVec2 barSize,
                     std::vector<glm::vec4> const &stops) {
  auto constexpr gradientSegments{256};

  for (auto const idx : iter::range(gradientSegments)) {
    auto const tLinear{gsl::narrow<float>(idx) /
                       gsl::narrow<float>(gradientSegments)};
    auto const tNextLinear{gsl::narrow<float>(idx + 1) /
                           gsl::narrow<float>(gradientSegments)};

    auto const color1{interpolateColor(stops, tLinear)};
    auto const color2{interpolateColor(stops, tNextLinear)};

    auto const x1{barPos.x + (tLinear * barSize.x)};
    auto const x2{barPos.x + (tNextLinear * barSize.x)};

    drawList->AddRectFilledMultiColor(
        ImVec2(x1, barPos.y), ImVec2(x2, barPos.y + barSize.y),
        colorToImU32(color1), colorToImU32(color2), colorToImU32(color2),
        colorToImU32(color1));
  }

  // Draw border
  auto constexpr borderThickness{1.5f};
  drawList->AddRect(barPos, ImVec2(barPos.x + barSize.x, barPos.y + barSize.y),
                    IM_COL32(150, 150, 150, 128), 0.0f, 0, borderThickness);
}

bool handleStopMarkerInteraction(ImGuiWindow *window, ImGuiID baseId,
                                 std::size_t idx, ImRect const &markerBb,
                                 glm::vec4 &stopColor, bool useAlpha) {
  auto const markerId{baseId + gsl::narrow<ImGuiID>(idx + 1)};
  bool valueChanged{false};

  if (ImGui::IsMouseHoveringRect(markerBb.Min, markerBb.Max)) {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      ImGui::SetActiveID(markerId, window);
    }
  }

  auto const popupId{std::format("##popupGradientColorPicker{}", idx)};

  if (ImGui::GetActiveID() == markerId &&
      ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    ImGui::OpenPopup(popupId.c_str());
  }

  if (ImGui::BeginPopup(popupId.c_str())) {
    std::array color{stopColor.r, stopColor.g, stopColor.b, stopColor.a};

    auto const flags{ImGuiColorEditFlags_DisplayRGB |
                     ImGuiColorEditFlags_DisplayHSV |
                     (useAlpha ? ImGuiColorEditFlags_AlphaBar |
                                     ImGuiColorEditFlags_AlphaPreview
                               : ImGuiColorEditFlags_None)};

    if (ImGui::ColorPicker4("##colorPicker", color.data(), flags)) {
      stopColor.r = color[0];
      stopColor.g = color[1];
      stopColor.b = color[2];
      stopColor.a = color[3];
      valueChanged = true;
    }

    ImGui::EndPopup();
  }

  return valueChanged;
}

bool drawStopMarkers(ImDrawList *drawList, ImGuiWindow *window, ImGuiID baseId,
                     std::vector<glm::vec4> &stops, ImVec2 barPos,
                     ImVec2 barSize, float markerSize, bool useAlpha) {
  bool valueChanged{false};

  for (auto const idx : iter::range(stops.size())) {
    auto const tStop{gsl::narrow<float>(idx) /
                     gsl::narrow<float>(stops.size() - 1)};

    auto const markerX{barPos.x + (tStop * barSize.x)};
    auto const markerY{barPos.y + ((barSize.y - markerSize) * 0.5f)};

    auto const markerPos{ImVec2(markerX - (markerSize * 0.5f), markerY)};
    auto const markerMax{
        ImVec2(markerX + (markerSize * 0.5f), markerY + markerSize)};

    // Draw marker
    drawList->AddRectFilled(markerPos, markerMax, colorToImU32(stops[idx]),
                            2.0f);
    drawList->AddRect(markerPos, markerMax, IM_COL32(0, 0, 0, 255), 2.0f, 0,
                      2.0f);

    // Handle interaction
    auto const markerBb{ImRect(markerPos, markerMax)};
    if (handleStopMarkerInteraction(window, baseId, idx, markerBb, stops[idx],
                                    useAlpha)) {
      valueChanged = true;
    }
  }

  return valueChanged;
}

void drawTickerMarks(ImDrawList *drawList, ImVec2 barPos, ImVec2 barSize,
                     int numTickers,
                     std::function<float(float)> const &tickerMapFunc) {
  if (numTickers <= 1) {
    return;
  }

  auto constexpr tickerMarkHeight{4.0f};
  auto const tickerY{barPos.y + barSize.y + 2.0f};
  auto const smallFontSize{ImGui::GetFontSize() * 0.95f};
  auto *const font{ImGui::GetFont()};
  auto const tickerColor{IM_COL32(200, 200, 200, 255)};

  for (auto const i : iter::range(numTickers)) {
    auto const tLinear{gsl::narrow<float>(i) /
                       gsl::narrow<float>(numTickers - 1)};
    auto const tMapped{tickerMapFunc ? tickerMapFunc(tLinear) : tLinear};

    auto const tickerX{barPos.x + (tLinear * barSize.x)};

    drawList->AddLine(ImVec2(tickerX, tickerY),
                      ImVec2(tickerX, tickerY + tickerMarkHeight), tickerColor,
                      1.0f);

    auto const valueText{std::format("{:.2g}", tMapped)};
    auto const textSize{
        font->CalcTextSizeA(smallFontSize, FLT_MAX, 0.0f, valueText.c_str())};
    auto const textPos{ImVec2(tickerX - (textSize.x * 0.5f),
                              tickerY + tickerMarkHeight + 1.0f)};

    drawList->AddText(font, smallFontSize, textPos, tickerColor,
                      valueText.c_str());
  }
}

} // namespace

bool uiWidgets::gradientWidget(char const *label, std::vector<glm::vec4> &stops,
                               bool useAlpha, ImVec2 size,
                               std::function<float(float)> const &tickerMapFunc,
                               bool showTickers, int numTickers,
                               char const *leftLabel, char const *centerLabel,
                               char const *rightLabel) {
  Expects(stops.size() >= 2);

  auto *const window{ImGui::GetCurrentWindow()};
  if (window->SkipItems) {
    return false;
  }

  auto const &style{GImGui->Style};
  auto const id{window->GetID(label)};

  // Calculate layout
  auto const hasLabels{*leftLabel != '\0' || *centerLabel != '\0' ||
                       *rightLabel != '\0'};
  auto const dims{calculateDimensions(size, hasLabels, showTickers)};
  auto const pos{window->DC.CursorPos};
  auto const positions{calculatePositions(pos, size, dims)};

  // Register item
  auto const totalBb{ImRect(pos, ImVec2(pos.x + size.x, pos.y + size.y))};
  ImGui::ItemSize(totalBb, style.FramePadding.y);
  if (!ImGui::ItemAdd(totalBb, id)) {
    return false;
  }

  auto *const drawList{window->DrawList};

  // Draw all components
  drawGradientLabels(drawList, positions.labelPos, size, leftLabel, centerLabel,
                     rightLabel, dims.labelHeight);

  if (useAlpha) {
    drawCheckerboard(drawList, positions.barPos, positions.barSize);
  }

  drawGradientBar(drawList, positions.barPos, positions.barSize, stops);

  bool const valueChanged{drawStopMarkers(drawList, window, id, stops,
                                          positions.barPos, positions.barSize,
                                          dims.markerSize, useAlpha)};

  if (showTickers) {
    drawTickerMarks(drawList, positions.barPos, positions.barSize, numTickers,
                    tickerMapFunc);
  }

  return valueChanged;
}

template <std::size_t N>
std::size_t uiWidgets::combo(char const *label,
                             std::array<char const *, N> items,
                             std::size_t currentIndex) {
  if (ImGui::BeginCombo(label, items.at(currentIndex))) {
    for (auto const index : iter::range(items.size())) {
      auto const isSelected{currentIndex == index};
      if (ImGui::Selectable(items.at(index), isSelected)) {
        currentIndex = index;
      }
      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  return currentIndex;
}

// Explicit instantiation for common sizes
template std::size_t
uiWidgets::combo<2>(char const *, std::array<char const *, 2>, std::size_t);
template std::size_t
uiWidgets::combo<3>(char const *, std::array<char const *, 3>, std::size_t);
template std::size_t
uiWidgets::combo<4>(char const *, std::array<char const *, 4>, std::size_t);
template std::size_t
uiWidgets::combo<5>(char const *, std::array<char const *, 5>, std::size_t);
template std::size_t
uiWidgets::combo<6>(char const *, std::array<char const *, 6>, std::size_t);