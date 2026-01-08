/**
 * @file appcontext.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef APPCONTEXT_HPP_
#define APPCONTEXT_HPP_

#include "appstate.hpp"
#include "functionmanager.hpp"
#include "renderstate.hpp"

struct AppContext {
  RenderState renderState;
  AppState appState;
  FunctionManager functionManager;
};

#endif