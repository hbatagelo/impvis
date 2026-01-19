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

#include <string>

namespace emscriptenMathJax {

bool updateEquation(std::string const &equation, std::string const &comment);
bool updateEquationName(std::string const &name);
bool setMathJaxVisibility(bool visible);

} // namespace emscriptenMathJax

#endif