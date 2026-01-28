/**
 * @file ui_tabs.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImVis is released under the MIT license.
 */

#ifndef UI_TABS_HPP_
#define UI_TABS_HPP_

struct AppContext;
class Camera;
class Raycast;

namespace uiTabs {

void functionsTab(AppContext &context, Camera &camera,
                  float parentWindowHeight);
void settingsTab(AppContext &context, Camera &camera);
void aboutTab(AppContext &context, Raycast const &raycast);

} // namespace uiTabs

#endif