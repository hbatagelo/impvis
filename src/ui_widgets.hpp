/**
 * @file ui_widgets.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef UI_WIDGETS_HPP_
#define UI_WIDGETS_HPP_

#include <glm/glm.hpp>
#include <imgui.h>

#include <functional>
#include <string_view>

struct AppContext;

namespace UIWidgets {

void showDelayedTooltip(std::string_view text, bool allowWhenDisabled = false);
void showRecommendedSettingsTooltip(AppContext &context);
void drawCheckerboard(ImDrawList *const drawList, ImVec2 pos, ImVec2 size,
                      float const checkerSize = 6.0f);
bool gradientWidget(std::string_view label, std::vector<glm::vec4> &stops,
                    bool useAlpha = true, ImVec2 size = ImVec2(300, 80),
                    std::function<float(float)> const &tickerMapFunc = nullptr,
                    bool showTickers = false, int numTickers = 11,
                    std::string_view leftLabel = "",
                    std::string_view centerLabel = "",
                    std::string_view rightLabel = "");

template <std::size_t N>
std::size_t combo(std::string_view label, std::array<std::string_view, N> items,
                  std::size_t currentIndex);

} // namespace UIWidgets

#endif