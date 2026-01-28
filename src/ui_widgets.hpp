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

struct AppContext;

namespace uiWidgets {

void showDelayedTooltip(char const *text, bool allowWhenDisabled = false);
void showRecommendedSettingsTooltip(AppContext &context);
void drawCheckerboard(ImDrawList *drawList, ImVec2 pos, ImVec2 size,
                      float checkerSize = 6.0f);
bool gradientWidget(char const *label, std::vector<glm::vec4> &stops,
                    bool useAlpha = true, ImVec2 size = ImVec2(300, 80),
                    std::function<float(float)> const &tickerMapFunc = nullptr,
                    bool showTickers = false, int numTickers = 11,
                    char const *leftLabel = "", char const *centerLabel = "",
                    char const *rightLabel = "");

template <std::size_t N>
std::size_t combo(char const *label, std::array<char const *, N> items,
                  std::size_t currentIndex);

} // namespace uiWidgets

#endif