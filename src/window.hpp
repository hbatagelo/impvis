/**
 * @file window.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#ifndef WINDOW_HPP_
#define WINDOW_HPP_

#include "background.hpp"
#include "equation.hpp"
#include "raycast.hpp"
#include "settings.hpp"
#include "textureblit.hpp"
#include "util.hpp"

#include <imgui.h>

class Window : public abcg::OpenGLWindow {
protected:
  void onEvent(SDL_Event const &event) override;
  void onCreate() override;
  void onPaint() override;
  void onPaintUI() override;
  void onResize(glm::ivec2 const &size) override;
  void onDestroy() override;

private:
  struct EquationGroup {
    std::string name;
    std::vector<Equation> equations;
  };

  void paintUIMainWindow();
  void paintUITopButtonBar();
  void paintUIEquationsTab(float parentWindowHeight);
  void paintUIEquationHeader(std::size_t groupIndex, EquationGroup &group);
  void paintUISettingsTab();
  void paintUIGeometryComboBox();
  void paintUIMethodComboBox();
  void paintUIRootTestComboBox();
  void paintUIShaderComboBox();
  void paintUIAboutTab();
  void paintUIEquationEditor();
  void paintUIIsoValueWindow();
  void paintUIParserDebugInfo();

  Settings m_settings{};

  ImFont *m_proportionalFont{};
  ImFont *m_monospacedFont{};
  GLuint m_backgroundRenderTex{};
  GLuint m_implicitSurfaceRenderTex{};
  std::array<GLuint, 4> m_buttonTexture{};
  TextureBlit m_textureBlit;
  Background m_background;
  RayCast m_rayCast;

  std::vector<EquationGroup> m_equationGroups;
};

#endif
