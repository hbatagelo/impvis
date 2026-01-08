/**
 * @file main.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "window.hpp"

int main(int argc, char **argv) {
  try {
    abcg::Application app(argc, argv);
    Window window;
    window.setWindowSettings({.width = 1024,
                              .height = 1024,
                              .showFPS = false,
                              .showFullscreenButton = false,
                              .fullscreenElementID = "#container",
                              .title = "ImpVis - 3D Implicit Function Viewer"});
    app.run(window);
  } catch (abcg::Exception const &exception) {
    fmt::print(stderr, "{}\n", exception.what());
    return -1;
  }
  return 0;
}