/**
 * @file ui_editor.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImVis is released under the MIT license.
 */

#ifndef UI_EDITOR_HPP_
#define UI_EDITOR_HPP_

#include <gsl/gsl>

struct ImFont;
struct AppContext;
class Raycast;

namespace UIEditor {

void functionEditor(AppContext &context, Raycast const &raycast,
                    gsl::not_null<ImFont*> font);

} // namespace UIEditor

#endif