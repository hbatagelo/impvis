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

void UIWidgets::showDelayedTooltip(std::string_view text,
                                   bool allowWhenDisabled) {
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal |
                           (allowWhenDisabled
                                ? ImGuiHoveredFlags_AllowWhenDisabled
                                : ImGuiHoveredFlags_None))) {
    ImGui::SetTooltip("%s", text.data());
  }
}

void UIWidgets::showRecommendedSettingsTooltip(AppContext &context) {
  if (context.appState.useRecommendedSettings) {
    showDelayedTooltip("Overridden by recommended settings", true);
  }
}

void UIWidgets::drawCheckerboard(ImDrawList *const drawList, ImVec2 pos,
                                 ImVec2 size, float const checkerSize) {
  auto const color1{IM_COL32(204, 204, 204, 255)};
  auto const color2{IM_COL32(153, 153, 153, 255)};

  auto const cols{gsl::narrow_cast<int>(std::ceil(size.x / checkerSize))};
  auto const rows{gsl::narrow_cast<int>(std::ceil(size.y / checkerSize))};

  for (auto const [row, col] :
       iter::product(iter::range(rows), iter::range(cols))) {
    auto const color{((row + col) % 2 == 0) ? color1 : color2};

    auto const minCorner{ImVec2(pos.x + gsl::narrow<float>(col) * checkerSize,
                                pos.y + gsl::narrow<float>(row) * checkerSize)};
    auto const maxCorner{
        ImVec2(std::min(pos.x + gsl::narrow<float>(col + 1) * checkerSize,
                        pos.x + size.x),
               std::min(pos.y + gsl::narrow<float>(row + 1) * checkerSize,
                        pos.y + size.y))};

    drawList->AddRectFilled(minCorner, maxCorner, color);
  }
}

bool UIWidgets::gradientWidget(
    std::string_view label, std::vector<glm::vec4> &stops, bool useAlpha,
    ImVec2 size, std::function<float(float)> const &tickerMapFunc,
    bool showTickers, int numTickers, std::string_view leftLabel,
    std::string_view centerLabel, std::string_view rightLabel) {
  Expects(stops.size() >= 2);

  auto *const window{ImGui::GetCurrentWindow()};
  if (window->SkipItems) {
    return false;
  }

  auto const &style{GImGui->Style};
  auto const id{window->GetID(label.data())};

  auto const toImU32{[](glm::vec4 color) -> ImU32 {
    return IM_COL32(gsl::narrow_cast<int>(color.r * 255.0f),
                    gsl::narrow_cast<int>(color.g * 255.0f),
                    gsl::narrow_cast<int>(color.b * 255.0f),
                    gsl::narrow_cast<int>(color.a * 255.0f));
  }};

  auto const getColorAt{[&stops](float t) -> glm::vec4 {
    if (stops.size() == 1) {
      return stops[0];
    }

    auto const tStop{t * gsl::narrow<float>(stops.size() - 1)};
    auto const idx{std::clamp(gsl::narrow<std::size_t>(std::floor(tStop)),
                              std::size_t{}, stops.size() - 2)};

    auto const tLocal{tStop - gsl::narrow<float>(idx)};
    return glm::mix(stops.at(idx), stops.at(idx + 1), tLocal);
  }};

  // Calculate sizes
  auto const hasLabels{!leftLabel.empty() || !centerLabel.empty() ||
                       !rightLabel.empty()};
  auto const smallFontSize{ImGui::GetFontSize() * 0.95f};
  auto const labelHeight{hasLabels ? smallFontSize + 4.0f : 0.0f};
  auto constexpr tickerMarkHeight{4.0f};
  auto const tickerHeight{showTickers ? tickerMarkHeight + smallFontSize + 3.0f
                                      : 0.0f};
  auto const markerSize{10.0f};
  auto const barHeight{size.y - labelHeight - tickerHeight};

  // Get position
  auto const pos{window->DC.CursorPos};
  auto const labelPos{ImVec2(pos.x, pos.y)};
  auto const barPos{ImVec2(pos.x, pos.y + labelHeight)};
  auto const barSize{ImVec2(size.x, barHeight)};

  // Register item
  auto const totalBb{ImRect(pos, ImVec2(pos.x + size.x, pos.y + size.y))};
  ImGui::ItemSize(totalBb, style.FramePadding.y);
  if (!ImGui::ItemAdd(totalBb, id)) {
    return false;
  }

  auto *const drawList{window->DrawList};

  // Draw labels at the top
  if (labelHeight > 0.0f) {
    auto *const font{ImGui::GetFont()};
    auto const offsetPosY{labelPos.y - 1};

    if (!leftLabel.empty()) {
      drawList->AddText(font, smallFontSize, ImVec2(labelPos.x, offsetPosY),
                        IM_COL32(200, 200, 200, 255), leftLabel.data());
    }

    if (!centerLabel.empty()) {
      auto const textSize{font->CalcTextSizeA(smallFontSize, FLT_MAX, 0.0f,
                                              centerLabel.data())};
      auto const centerPos{
          ImVec2(pos.x + (size.x - textSize.x) * 0.5f, offsetPosY)};
      drawList->AddText(font, smallFontSize, centerPos,
                        IM_COL32(200, 200, 200, 255), centerLabel.data());
    }

    if (!rightLabel.empty()) {
      auto const textSize{
          font->CalcTextSizeA(smallFontSize, FLT_MAX, 0.0f, rightLabel.data())};
      auto const rightPos{ImVec2(pos.x + size.x - textSize.x, offsetPosY)};
      drawList->AddText(font, smallFontSize, rightPos,
                        IM_COL32(200, 200, 200, 255), rightLabel.data());
    }
  }

  // Draw checkerboard background
  if (useAlpha) {
    drawCheckerboard(drawList, barPos, barSize);
  }

  // Draw gradient bar
  auto constexpr segments{256};
  for (auto const idx : iter::range(segments)) {
    auto const tLinear{gsl::narrow<float>(idx) / gsl::narrow<float>(segments)};
    auto const tNextLinear{gsl::narrow<float>(idx + 1) /
                           gsl::narrow<float>(segments)};

    auto const color1{getColorAt(tLinear)};
    auto const color2{getColorAt(tNextLinear)};

    auto const x1{barPos.x + tLinear * barSize.x};
    auto const x2{barPos.x + tNextLinear * barSize.x};

    drawList->AddRectFilledMultiColor(
        ImVec2(x1, barPos.y), ImVec2(x2, barPos.y + barSize.y), toImU32(color1),
        toImU32(color2), toImU32(color2), toImU32(color1));
  }

  // Draw border around bar
  drawList->AddRect(barPos, ImVec2(barPos.x + barSize.x, barPos.y + barSize.y),
                    IM_COL32(150, 150, 150, 128), 0.0f, 0, 1.5f);

  // Draw stop markers
  auto valueChanged{false};
  for (auto const idx : iter::range(stops.size())) {
    auto const tStop{gsl::narrow<float>(idx) /
                     gsl::narrow<float>(stops.size() - 1)};

    auto const markerX{barPos.x + tStop * barSize.x};
    auto const markerY{barPos.y + (barSize.y - markerSize) * 0.5f};

    auto const markerPos{ImVec2(markerX - markerSize * 0.5f, markerY)};
    auto const markerMax{
        ImVec2(markerX + markerSize * 0.5f, markerY + markerSize)};
    auto const markerBb{ImRect(markerPos, markerMax)};

    // Draw marker
    drawList->AddRectFilled(markerPos, markerMax, toImU32(stops[idx]), 2.0f);
    drawList->AddRect(markerPos, markerMax, IM_COL32(0, 0, 0, 255), 2.0f, 0,
                      2.0f);

    // Handle click on marker
    auto const markerId{id + gsl::narrow<ImGuiID>(idx + 1)};

    if (ImGui::IsMouseHoveringRect(markerBb.Min, markerBb.Max)) {
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImGui::SetActiveID(markerId, window);
      }
    }

    // Color picker popup
    auto const popupId{std::format("##popupGradientColorPicker{}", idx)};

    if (ImGui::GetActiveID() == markerId &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      ImGui::OpenPopup(popupId.c_str());
    }

    if (ImGui::BeginPopup(popupId.c_str())) {
      float color[4]{stops[idx].r, stops[idx].g, stops[idx].b, stops[idx].a};

      auto const flags{ImGuiColorEditFlags_DisplayRGB |
                       ImGuiColorEditFlags_DisplayHSV |
                       (useAlpha ? ImGuiColorEditFlags_AlphaBar |
                                       ImGuiColorEditFlags_AlphaPreview
                                 : ImGuiColorEditFlags_None)};

      if (ImGui::ColorPicker4("##colorPicker", color, flags)) {
        stops[idx].r = color[0];
        stops[idx].g = color[1];
        stops[idx].b = color[2];
        stops[idx].a = color[3];
        valueChanged = true;
      }

      ImGui::EndPopup();
    }
  }

  // Draw ticker marks if enabled
  if (showTickers && numTickers > 1) {
    auto const tickerY{barPos.y + barSize.y + 2.0f};

    auto *const font{ImGui::GetFont()};

    for (auto const i : iter::range(numTickers)) {
      auto const tLinear{gsl::narrow<float>(i) /
                         gsl::narrow<float>(numTickers - 1)};
      auto const tMapped{tickerMapFunc ? tickerMapFunc(tLinear) : tLinear};

      auto const tickerX{barPos.x + tLinear * barSize.x};

      drawList->AddLine(ImVec2(tickerX, tickerY),
                        ImVec2(tickerX, tickerY + tickerMarkHeight),
                        IM_COL32(200, 200, 200, 255), 1.0f);

      auto const valueText{std::format("{:.2g}", tMapped)};
      auto const textSize{
          font->CalcTextSizeA(smallFontSize, FLT_MAX, 0.0f, valueText.c_str())};
      auto const textPos{ImVec2(tickerX - textSize.x * 0.5f,
                                tickerY + tickerMarkHeight + 1.0f)};

      drawList->AddText(font, smallFontSize, textPos,
                        IM_COL32(200, 200, 200, 255), valueText.c_str());
    }
  }

  return valueChanged;
}

template <std::size_t N>
std::size_t UIWidgets::combo(std::string_view label,
                             std::array<std::string_view, N> items,
                             std::size_t currentIndex) {
  if (ImGui::BeginCombo(label.data(), items.at(currentIndex).data())) {
    for (auto const index : iter::range(items.size())) {
      auto const isSelected{currentIndex == index};
      if (ImGui::Selectable(items[index].data(), isSelected)) {
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
template std::size_t UIWidgets::combo<2>(std::string_view,
                                         std::array<std::string_view, 2>,
                                         std::size_t);
template std::size_t UIWidgets::combo<3>(std::string_view,
                                         std::array<std::string_view, 3>,
                                         std::size_t);
template std::size_t UIWidgets::combo<4>(std::string_view,
                                         std::array<std::string_view, 4>,
                                         std::size_t);
template std::size_t UIWidgets::combo<5>(std::string_view,
                                         std::array<std::string_view, 5>,
                                         std::size_t);
template std::size_t UIWidgets::combo<6>(std::string_view,
                                         std::array<std::string_view, 6>,
                                         std::size_t);