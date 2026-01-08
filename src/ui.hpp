/**
 * @file ui.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImVis is released under the MIT license.
 */

#ifndef UI_HPP_
#define UI_HPP_

#include "renderpipeline.hpp"

struct AppContext;
class Camera;
class Raycast;
class RenderPipeline;

class UI {
public:
  static inline std::optional<std::monostate> s_noEquation;

  void onCreate(AppContext const &context);
  void onPaint();
  void onPaintUI(AppContext &context, RenderPipeline &pipeline, Camera &camera);
  void onDestroy();

  ImFont *getProportionalFont() const { return m_proportionalFont; }
  ImFont *getMonospacedFont() const { return m_monospacedFont; }
  ImFont *getSmallFont() const { return m_smallFont; }

private:
  void mainWindow(AppContext &context, Camera &camera, Raycast const &raycast);
  void topButtonBar(AppContext &context);
  void isoValueWindow(AppContext &context);
  void progressIndicator(ImVec2 position, float width, Raycast const &raycast);
  void updateEquation(AppContext const &context, bool includeName = false);
  void surfaceInfoTooltip(RenderPipeline &pipeline, AppContext const &context);

  ImFont *m_proportionalFont{};
  ImFont *m_monospacedFont{};
  ImFont *m_smallFont{};

  SDL_Cursor *m_crossHairCursor{};

  std::vector<GLuint> m_buttonTextures;
  std::optional<RenderPipeline::PixelData> m_lastPixelData{};
};

#endif