/**
 * @file ui_emscripten.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef UI_EMSCRIPTEN_HPP_
#define UI_EMSCRIPTEN_HPP_

#include <string_view>

namespace emscriptenMathJax {

bool updateEquation(std::string_view equation, std::string_view comment);
bool updateEquationName(std::string_view name);
bool setMathJaxVisibility(bool visible);

} // namespace emscriptenMathJax

#endif