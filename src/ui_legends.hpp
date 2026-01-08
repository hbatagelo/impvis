/**
 * @file ui_legends.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef UI_LEGENDS_HPP_
#define UI_LEGENDS_HPP_

struct AppContext;

namespace UILegends {

void isovalueLegendAndModeSettings(AppContext &context);
void dvrLegendAndModeSettings(AppContext &context);
void normalLegendAndModeSettings(AppContext &context);
void curvatureLegendAndModeSettings(AppContext &context);

} // namespace UILegends

#endif