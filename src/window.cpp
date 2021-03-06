
/**
 * @file window.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#include <cppitertools/itertools.hpp>
#include <fmt/core.h>
#include <imgui.h>
#include <memory>

#include "abcg_application.hpp"

#include "abcg_openglfunctions.hpp"
#include "equation.hpp"
#include "imgui_internal.h"
#include "settings.hpp"
#include "util.hpp"
#include "window.hpp"

static char const *const kAppVersion{"v2.0.0"};

#if defined(__EMSCRIPTEN__)
extern "C" {
EMSCRIPTEN_KEEPALIVE
float my_sqrt(float x) { return sqrt(x); }
}
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

EM_JS(void, jsUpdateEquation,
      (char const *equation, int equationLength, char const *comment,
       int commentLength),
      {
        updateEquation(UTF8ToString(equation, equationLength),
                       UTF8ToString(comment, commentLength));
      });

EM_JS(void, jsUpdateEquationName, (char const *name, int length),
      { updateEquationName(UTF8ToString(name, length)); });

bool updateEquation(std::string equation, std::string comment) {
  jsUpdateEquation(equation.c_str(), equation.length(), comment.c_str(),
                   comment.length());
  return true;
}

bool updateEquationName(std::string name) {
  jsUpdateEquationName(name.c_str(), name.length());
  return true;
}

EMSCRIPTEN_BINDINGS(module) {
  emscripten::function("updateEquation", &updateEquation);
  emscripten::function("updateEquationName", &updateEquationName);
}
#endif

void Window::handleEvent(SDL_Event &event) {
  if (!m_settings.drawUI) {
    if (event.type == SDL_KEYUP) {
      m_settings.drawUI = true;
    }
  }

  m_rayCast.handleEvent(event);
}

void Window::initializeGL() {
  auto const assetsPath{abcg::Application::getAssetsPath()};

  // Load equations from a catalogue file
  auto loadEqn{[&](std::string_view filename) {
    return Equation::loadCatalogue(assetsPath + "equations/" +
                                   std::string{filename});
  }};
  m_equationGroups.push_back(EquationGroup{"Cubic", loadEqn("cubic.txt")});
  m_equationGroups.push_back(EquationGroup{"Quartic", loadEqn("quartic.txt")});
  m_equationGroups.push_back(EquationGroup{"Quintic", loadEqn("quintic.txt")});
  m_equationGroups.push_back(EquationGroup{"Sextic", loadEqn("sextic.txt")});
  m_equationGroups.push_back(EquationGroup{"Septic", loadEqn("septic.txt")});
  m_equationGroups.push_back(EquationGroup{"Octic", loadEqn("octic.txt")});
  m_equationGroups.push_back(EquationGroup{"Nonic", loadEqn("nonic.txt")});
  m_equationGroups.push_back(
      EquationGroup{"Decic and higher", loadEqn("decp.txt")});
  m_equationGroups.push_back(
      EquationGroup{"Non-algebraic", loadEqn("nonalg.txt")});
  m_equationGroups.push_back(
      EquationGroup{"User-defined", std::vector<Equation>{Equation{}}});

  for (auto &equationGroup : m_equationGroups) {
    for (auto &equation : equationGroup.equations) {
      equation.initializeGL();
    }
  }

  // Update selected equation
  auto const equation{m_equationGroups.at(m_settings.selectedUIGroupIndex)
                          .equations.at(m_settings.selectedUIEquationIndex)};
  m_settings.equation = equation;

  auto const &loadedData{m_settings.equation.getLoadedData()};

  if (m_settings.usePredefinedSettings) {
    m_rayCast.setLookAtDistance(loadedData.camDist);
  }

#if defined(__EMSCRIPTEN__)
  updateEquationName(loadedData.name);
  updateEquation(equation.getMathJaxExpression(m_settings.isoValue),
                 m_settings.overlayMathJaxComment ? loadedData.comment : "");
#endif

  m_background.initializeGL();
  m_textureBlit.initializeGL();
  m_rayCast.initializeGL(m_settings);

  for (auto const &index : iter::range(m_buttonTexture.size())) {
    m_buttonTexture.at(index) = abcg::opengl::loadTexture(
        assetsPath + fmt::format("textures/top_button{}.png", index), false,
        false);
  }

  abcg::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  abcg::glDisable(GL_DEPTH_TEST);
  abcg::glEnable(GL_BLEND);
  abcg::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  ImGuiIO &guiIO{ImGui::GetIO()};
  // Blink caret in input text widgets
  guiIO.ConfigInputTextCursorBlink = true;

  // Load fonts
  ImFontConfig fontConfig;
  auto const fontSize{16.0f};

  auto const *proportionalFontFile{"fonts/Roboto-Medium.ttf"};
  if (m_proportionalFont = guiIO.Fonts->AddFontFromFileTTF(
          (assetsPath + proportionalFontFile).c_str(), fontSize, &fontConfig);
      m_proportionalFont == nullptr) {
    throw abcg::RunTimeError(
        fmt::format("Failed to load {}", proportionalFontFile));
  }

  auto const *monospacedFontFile{"fonts/RobotoMono-Regular.ttf"};
  if (m_monospacedFont = guiIO.Fonts->AddFontFromFileTTF(
          (assetsPath + monospacedFontFile).c_str(), fontSize, &fontConfig);
      m_monospacedFont == nullptr) {
    throw abcg::RunTimeError(
        fmt::format("Failed to load {}", monospacedFontFile));
  }
}

void Window::paintGL() {
  abcg::glViewport(0, 0, static_cast<GLsizei>(m_settings.viewportSize.x),
                   static_cast<GLsizei>(m_settings.viewportSize.y));

  if (m_settings.drawBackground) {
    if (m_settings.redrawBackgroundRenderTex) {
      m_background.paintGL(m_backgroundRenderTex);
      m_settings.redrawBackgroundRenderTex = false;
    }
    m_textureBlit.draw(m_backgroundRenderTex);
  }

  // Override settings?
  if (m_settings.usePredefinedSettings) {
    auto const &loadedData{m_settings.equation.getLoadedData()};
    m_settings.renderSettings.useBoundingBox = loadedData.boundShape == 1;
    m_settings.renderSettings.boundRadius = loadedData.boundRadius;
    m_settings.renderSettings.rayMarchAdaptive = loadedData.rayMarchMethod == 0;
    m_settings.renderSettings.rayMarchSignTest =
        loadedData.rayMarchRootTest == 0;
    m_settings.colormapScale = loadedData.colormapScale;

    // Reduce number of steps when using predefined settings with no shadows
    auto rayMarchSteps{loadedData.rayMarchSteps};
    if (!m_settings.renderSettings.useShadows) {
      if (rayMarchSteps > 60) {
        auto const scaleFactor{0.75f};
        rayMarchSteps =
            static_cast<int>(static_cast<float>(rayMarchSteps) * scaleFactor);
      }
    }
    m_settings.renderSettings.rayMarchSteps = rayMarchSteps;
  }

  // Rebuild the ray casting shader program if the settings have changed since
  // the last call to paintGL
  static Settings lastSettings{};
  m_settings.rebuildProgram |=
      lastSettings.renderSettings != m_settings.renderSettings ||
      lastSettings.selectedUIGroupIndex != m_settings.selectedUIGroupIndex ||
      lastSettings.selectedUIEquationIndex !=
          m_settings.selectedUIEquationIndex;

  if (m_settings.drawBackground && !m_settings.takeScreenshot) {
    static auto const KaIaRed{glm::vec4{0.12f, 0.12f, 0.25f, 1.0f}};
    m_rayCast.setKaIa(KaIaRed);
    m_rayCast.paintGL(m_settings, m_implicitSurfaceRenderTex);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_textureBlit.draw(m_implicitSurfaceRenderTex);

    glDisable(GL_BLEND);
  } else {
    abcg::glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    static auto const KaIaBlue{glm::vec4{0.075f, 0.04f, 0.04f, 1.0f}};
    m_rayCast.setKaIa(KaIaBlue);
    m_rayCast.paintGL(m_settings);

    glDisable(GL_BLEND);
    abcg::glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (m_settings.takeScreenshot) {
      saveScreenshotPNG("screenshot.png");
      m_settings.takeScreenshot = false;
    }
  }

  m_settings.rebuildProgram = false;
  lastSettings = m_settings;
}

void Window::paintUI() {
  abcg::OpenGLWindow::paintUI();

  // Disable UI when taking screenshots
  if (m_settings.takeScreenshot || !m_settings.drawUI)
    return;

  ImGui::PushFont(m_proportionalFont);

  paintMainWindow();

  if (m_settings.showEquationEditor) {
    paintEquationEditor();
  }

  paintIsoValueWindow();

  // Refresh equation rendering using MathJax
  static auto lastElapsedTime{0.0};
  if (auto const timeOut{0.125}; getElapsedTime() - lastElapsedTime > timeOut) {
    lastElapsedTime = getElapsedTime();
    static auto lastIsoValue{0.0f};
    if (lastIsoValue != m_settings.isoValue) {
#if defined(__EMSCRIPTEN__)
      auto const &loadedData{m_settings.equation.getLoadedData()};
      updateEquation(
          m_settings.equation.getMathJaxExpression(m_settings.isoValue),
          m_settings.overlayMathJaxComment ? loadedData.comment : "");
#endif
      lastIsoValue = m_settings.isoValue;
    }
  }

  ImGui::PopFont();
}

void Window::paintMainWindow() {
  // Create main window widget
  auto minWindowSize{ImVec2(248, 534)};
  auto maxWindowSize{minWindowSize};
  auto parametersExtraHeight{0UL};
  auto const &parameters{m_settings.equation.getParameters()};
  if (!parameters.empty()) {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267) // Safe conversion from size_t to unsigned long
#endif
    parametersExtraHeight = 34 + parameters.size() * 26;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
  }

  ImVec2 windowSize{
      minWindowSize.x,
      std::max(minWindowSize.y,
               std::min(maxWindowSize.y, m_settings.viewportSize.y * 0.5f))};

  ImGui::SetNextWindowPos(
      ImVec2(m_settings.viewportSize.x - windowSize.x - 5, 5));
  ImGui::SetNextWindowSize(windowSize);

  ImGui::Begin("ImpVis", nullptr,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove);

  paintTopButtonBar();

  if (ImGui::BeginTabBar("##main_tab_bar", ImGuiTabBarFlags_None)) {
    if (ImGui::BeginTabItem("Equations")) {
      paintEquationsTab(windowSize.y - 63);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Settings")) {
      paintSettingsTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("About")) {
      paintAboutTab();
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
  ImGui::End();

  // Parameters
  if (!parameters.empty() && m_rayCast.isProgramValid()) {
    ImGui::SetNextWindowPos(ImVec2(m_settings.viewportSize.x - windowSize.x - 5,
                                   windowSize.y + 10));
    ImGui::SetNextWindowSize(
        ImVec2(windowSize.x, static_cast<float>(parametersExtraHeight)));
    ImGui::Begin("Parameters", nullptr,
                 ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove);

    auto const step{0.01f};
    auto const spacing{6};
    for (auto const &parameter : parameters) {
      auto value{parameter.value};

      // Left arrow button
      ImGui::PushButtonRepeat(true);
      if (ImGui::ArrowButton(fmt::format("##lpar_{}", parameter.name).c_str(),
                             ImGuiDir_Left)) {
        value -= step;
      }
      ImGui::PopButtonRepeat();

      // Drag slider
      ImGui::SameLine(0.0f, spacing);
      ImGui::PushItemWidth(178);
      auto const label{fmt::format("##par_{}", parameter.name)};
      auto const format{
          fmt::format("{}: {:.2f}", parameter.name, parameter.value)};
      ImGui::DragFloat(label.c_str(), &value, 0.01f, 0.f, 0.f, format.c_str(),
                       ImGuiSliderFlags_NoRoundToFormat);
      if (ImGui::IsItemHovered() && !ImGui::IsAnyMouseDown())
        ImGui::SetTooltip("Drag to\nchange");
      ImGui::PopItemWidth();

      // Right arrow button
      ImGui::SameLine(0.0f, spacing);
      ImGui::PushButtonRepeat(true);
      if (ImGui::ArrowButton(fmt::format("##rpar_{}", parameter.name).c_str(),
                             ImGuiDir_Right)) {
        value += step;
      }
      ImGui::PopButtonRepeat();

      m_settings.equation.setParameter(parameter.name, value);
    }

    ImGui::End();
  }

  if (m_settings.showParserDebugInfo) {
    paintParserDebugInfo();
  }
}

void Window::paintTopButtonBar() {
  auto isSelected{[this](std::size_t buttonIndex) {
    auto const &renderSettings{m_settings.renderSettings};
    switch (buttonIndex) {
    // Isosurface unshadowed
    case 0:
      return renderSettings.shaderIndex == 0 &&
             !renderSettings.useNormalsAsColors && !renderSettings.useFog &&
             !renderSettings.useShadows;
    // Isovalue shadowed + fog
    case 1:
      return renderSettings.shaderIndex == 0 &&
             !renderSettings.useNormalsAsColors && renderSettings.useFog &&
             renderSettings.useShadows;
    // Scalar field
    case 2:
      return renderSettings.shaderIndex == 2;
    // Isovalue normal
    case 3:
      return renderSettings.useNormalsAsColors &&
             renderSettings.shaderIndex == 1;
    default:
      return false;
    }
  }};

  auto onClicked{[this](std::size_t buttonIndex) {
    auto &renderSettings{m_settings.renderSettings};
    switch (buttonIndex) {
    // Isovalue unshadowed
    case 0:
      renderSettings.shaderIndex = 0;
      renderSettings.useFog = false;
      renderSettings.useNormalsAsColors = false;
      renderSettings.useShadows = false;
      break;
    // Isovalue shadowed + fog
    case 1:
      renderSettings.shaderIndex = 0;
      renderSettings.useFog = true;
      renderSettings.useNormalsAsColors = false;
      renderSettings.useShadows = true;
      break;
    // Scalar field
    case 2:
      renderSettings.shaderIndex = 2;
      renderSettings.useFog = false;
      renderSettings.useShadows = false;
      break;
    // Isovalue normal
    case 3:
      renderSettings.shaderIndex = 1;
      renderSettings.useFog = true;
      renderSettings.useNormalsAsColors = true;
      renderSettings.useShadows = false;
      break;
    default:
      break;
    }
  }};

  auto onHovered{[](std::size_t buttonIndex) {
    std::string str{};
    switch (buttonIndex) {
    case 0:
      str = "Isosurface";
      break;
    case 1:
      str = "Isosurface +\nfog + shadows";
      break;
    case 2:
      str = "Scalar\nfield";
      break;
    case 3:
      str = "Normals\nas colors";
      break;
    default:
      break;
    }
    ImGui::SetTooltip("%s", str.c_str());
  }};

  auto const buttonSize{ImVec2(47, 47)};
  for (auto const index : iter::range(m_buttonTexture.size())) {
    auto const selected{isSelected(index)};
    if (selected) {
      auto color{ImVec4(0.62f, 0.62f, 0.62f, 1.0f)};
      ImGui::PushStyleColor(ImGuiCol_Button, color);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    } else {
      auto color{ImVec4(0.11f, 0.11f, 0.11f, 1.0f)};
      ImGui::PushStyleColor(ImGuiCol_Button, color);
    }

    auto const texture{static_cast<intptr_t>(m_buttonTexture.at(index))};
    auto *const texID{reinterpret_cast<ImTextureID>(texture)}; // NOLINT
    if (ImGui::ImageButton(texID, buttonSize)) {
      onClicked(index);
    }
    if (ImGui::IsItemHovered()) {
      onHovered(index);
    }

    if (selected) {
      ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 4);
  }
  ImGui::Spacing();
  ImGui::Spacing();
}

void Window::paintEquationsTab(float parentWindowHeight) {
  ImGui::BeginChild("##fn_child_window", ImVec2(0, parentWindowHeight - 93),
                    true, ImGuiWindowFlags_None);

  static auto firstTime{true};

  for (auto &&[groupIndex, group] : iter::enumerate(m_equationGroups)) {
    ImGuiTreeNodeFlags headerFlags{ImGuiTreeNodeFlags_CollapsingHeader};
    if (firstTime && m_settings.selectedUIGroupIndex == groupIndex) {
      ImGui::SetNextItemOpen(true);
      firstTime = false;
    }
    if (ImGui::CollapsingHeader(group.name.c_str(), headerFlags)) {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244) // From unsigned __int64 to unsigned long
#endif
      paintEquationHeader(groupIndex, group);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    }
  }

  ImGui::EndChild();

  ImGui::Spacing();
  auto showEquationEditor{m_settings.showEquationEditor};
  ImGui::Checkbox("Show equation editor", &showEquationEditor);
  if (showEquationEditor != m_settings.showEquationEditor) {
    m_settings.showEquationEditor = showEquationEditor;
    m_settings.overlayMathJaxComment = !showEquationEditor;
#if defined(__EMSCRIPTEN__)
    auto const &loadedData{m_settings.equation.getLoadedData()};
    updateEquation(
        m_settings.equation.getMathJaxExpression(m_settings.isoValue),
        m_settings.overlayMathJaxComment ? loadedData.comment : "");
#endif
  }
}

void Window::paintEquationHeader(unsigned long groupIndex,
                                 EquationGroup &group) {
  if (ImGui::BeginTable(fmt::format("##tbl{}", groupIndex).c_str(), 2,
                        ImGuiTableFlags_SizingFixedFit)) {
    ImVec2 const thumbSize{32, 32};
    for (auto &&[equationIndex, equation] : iter::enumerate(group.equations)) {
      if (equation.getLoadedData().name.empty()) {
        continue;
      }

      auto selected{m_settings.selectedUIGroupIndex == groupIndex &&
                    m_settings.selectedUIEquationIndex == equationIndex};
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      if (auto const thumbnailID{
              static_cast<intptr_t>(equation.getThumbnailId())};
          thumbnailID > 0) {
        auto *const texture{
            reinterpret_cast<ImTextureID>(thumbnailID)}; // NOLINT
        ImGui::Image(texture, thumbSize);
      }
      ImGui::TableNextColumn();
      auto const &loadedData{equation.getLoadedData()};
      ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,
                          ImVec2(0.0f, 0.45f));
      ImGui::Selectable(loadedData.name.c_str(), &selected,
                        ImGuiSelectableFlags_SpanAllColumns,
                        ImVec2(0, thumbSize.y - 2));
      ImGui::PopStyleVar(1);
      if (selected && (m_settings.selectedUIGroupIndex != groupIndex ||
                       m_settings.selectedUIEquationIndex != equationIndex)) {
        m_settings.selectedUIGroupIndex = groupIndex;
        m_settings.selectedUIEquationIndex = equationIndex;
        m_settings.equation = {
            m_equationGroups.at(groupIndex).equations.at(equationIndex)};
        if (m_settings.usePredefinedSettings) {
          m_rayCast.setLookAtDistance(loadedData.camDist);
        }
#if defined(__EMSCRIPTEN__)
        updateEquationName(loadedData.name);
        updateEquation(equation.getMathJaxExpression(m_settings.isoValue),
                       m_settings.overlayMathJaxComment ? loadedData.comment
                                                        : "");
#endif
      }
    }
    ImGui::EndTable();
  }
}

void Window::paintSettingsTab() {
  ImGui::Spacing();
  ImGui::Checkbox("Use recommended settings",
                  &m_settings.usePredefinedSettings);

  ImGui::Spacing();
  ImGui::Text("Bounding geometry:");
  ImGui::BeginChild("##bounds_child", ImVec2(0, 65), true);
  ImGui::PushItemWidth(170);

  if (m_settings.usePredefinedSettings) {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  }

  // Bounding geometry combo box
  paintGeometryComboBox();

  // Bounding radius
  ImGui::SliderFloat("Radius", &m_settings.renderSettings.boundRadius, 1, 20,
                     "%.1f");

  if (m_settings.usePredefinedSettings) {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
  }

  ImGui::PopItemWidth();
  ImGui::EndChild();

  ImGui::Text("Isosurface ray marching:");
  ImGui::BeginChild("##ray_marching_child", ImVec2(0, 91), true);

  ImGui::PushItemWidth(156);

  // Disable ray marching options when using predefined settings or scalar field
  // shader
  if (m_settings.usePredefinedSettings ||
      m_settings.renderSettings.shaderIndex == 2) {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  }

  // Method
  paintMethodComboBox();

  // Ray marching steps
  auto const minSteps{5};
  auto const maxSteps{1500};
  ImGui::SliderInt("Steps", &m_settings.renderSettings.rayMarchSteps, minSteps,
                   maxSteps);

  // Root test
  paintRootTestComboBox();

  ImGui::PopItemWidth();

  if (m_settings.usePredefinedSettings ||
      m_settings.renderSettings.shaderIndex == 2) {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
  }

  ImGui::EndChild();

  ImGui::Text("Lighting & shading:");
  ImGui::BeginChild("##lightingshading_child", ImVec2(0, 91), true);

  ImGui::PushItemWidth(168);

  // Shader combo box
  paintShaderComboBox();

  if (m_settings.renderSettings.shaderIndex != 2) // Scalar field
  {
    ImGui::Checkbox("Shadows", &m_settings.renderSettings.useShadows);
    ImGui::SameLine(0.0f, 64.0f);
    ImGui::Checkbox("Fog", &m_settings.renderSettings.useFog);
    ImGui::Checkbox("Normals as colors",
                    &m_settings.renderSettings.useNormalsAsColors);
    ImGui::PopItemWidth();
  } else {
    ImGui::PopItemWidth();
    ImGui::Text("Colormap scaling:");
    ImGui::PushItemWidth(216);
    ImGui::Spacing();

    if (m_settings.usePredefinedSettings) {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    ImGui::SliderFloat("##gaussianEps", &m_settings.colormapScale, 1e-7f, 20.0f,
                       "%.4g", ImGuiSliderFlags_Logarithmic);

    if (m_settings.usePredefinedSettings) {
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }

    ImGui::PopItemWidth();
  }

  ImGui::EndChild();

  ImGui::Spacing();
  ImGui::Checkbox("Show gradient background", &m_settings.drawBackground);

  ImGui::Spacing();
  if (ImGui::Button("Hide UI windows", ImVec2(-1, 0))) {
    m_settings.drawUI = false;
  }
}

void Window::paintGeometryComboBox() {
  std::size_t currentIndex{m_settings.renderSettings.useBoundingBox ? 0UL
                                                                    : 1UL};
  std::vector<std::string> const comboItems{"Box", "Sphere"};
  if (ImGui::BeginCombo("Shape", comboItems.at(currentIndex).c_str())) {
    for (auto const index : iter::range(comboItems.size())) {
      auto const isSelected{currentIndex == index};
      if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
        currentIndex = index;
      if (isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  m_settings.renderSettings.useBoundingBox = currentIndex == 0;
}

void Window::paintMethodComboBox() {
  std::size_t currentIndex{m_settings.renderSettings.rayMarchAdaptive ? 0UL
                                                                      : 1UL};
  std::vector<std::string> const comboItems{"Adaptive", "Fixed-step"};
  if (ImGui::BeginCombo("Method", comboItems.at(currentIndex).c_str())) {
    for (auto const index : iter::range(comboItems.size())) {
      auto const isSelected{currentIndex == index};
      if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
        currentIndex = index;
      if (isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  m_settings.renderSettings.rayMarchAdaptive = currentIndex == 0;
}

void Window::paintRootTestComboBox() {
  std::size_t currentIndex{m_settings.renderSettings.rayMarchSignTest ? 0UL
                                                                      : 1UL};
  std::vector<std::string> const comboItems{"Sign change", "Taylor 1st-order"};
  if (ImGui::BeginCombo("Root test", comboItems.at(currentIndex).c_str())) {
    for (auto const index : iter::range(comboItems.size())) {
      auto const isSelected{currentIndex == index};
      if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
        currentIndex = index;
      if (isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  m_settings.renderSettings.rayMarchSignTest = currentIndex == 0;
}

void Window::paintShaderComboBox() {
  std::size_t currentIndex{m_settings.renderSettings.shaderIndex};
  std::vector<std::string> const comboItems{
      "Shaded isosurface", "Unlit isosurface", "Scalar field (DVR)"};
  if (ImGui::BeginCombo("Shader", comboItems.at(currentIndex).c_str())) {
    for (auto const index : iter::range(comboItems.size())) {
      auto const isSelected{currentIndex == index};
      if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
        currentIndex = index;
      if (isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  m_settings.renderSettings.shaderIndex = currentIndex;
}

void Window::paintAboutTab() {
  ImGui::Text("%s", fmt::format("ImpVis {}", kAppVersion).c_str());
  ImGui::Text("3D Implicit Function Viewer");
  ImGui::Text("Copyright (c) 2022 Harlen Batagelo");

  ImGui::Spacing();
  ImGui::Spacing();

  // FPS counter
  ImGui::BeginChild("##statistics_child", ImVec2(0, 66), true);
  ImGui::PushItemWidth(168);
  {
    auto const fps{ImGui::GetIO().Framerate};

    static auto offset{0UL};
    static auto refreshTime{ImGui::GetTime()};
    static std::array<float, 218> frames{};

    while (refreshTime < ImGui::GetTime()) {
      constexpr auto refreshFrequency{60.0};
      frames.at(offset) = fps;
      offset = (offset + 1) % frames.size();
      refreshTime += (1.0 / refreshFrequency);
    }

    auto const label{fmt::format("{:.1f} FPS", fps)};
    ImGui::PlotLines("##fps", frames.data(), static_cast<int>(frames.size()),
                     static_cast<int>(offset), label.c_str(), 0.0f,
                     *std::max_element(frames.begin(), frames.end()) * 2,
                     ImVec2(static_cast<float>(frames.size()), 50));
  }
  ImGui::PopItemWidth();
  ImGui::EndChild();

  ImGui::Spacing();
  ImGui::Checkbox("Show parser debug info", &m_settings.showParserDebugInfo);

#if !defined(__EMSCRIPTEN__)
  ImGui::Spacing();
  if (ImGui::Button("Take screenshot", ImVec2(-1, 0))) {
    m_settings.takeScreenshot = true;
  }
#endif
}

void Window::paintEquationEditor() {
  ImVec2 windowSize{};
  ImVec2 const minWindowSize{250, 240};
  auto const inputTextHeight{ImGui::GetTextLineHeight() + 8};
  auto const &loadedData{m_settings.equation.getLoadedData()};

  // Window first appear centered in screen
  if (m_settings.updateEquationEditorLayout) {
    windowSize = ImVec2{std::max(m_settings.viewportSize.x - 10,
                                 m_settings.viewportSize.x * 0.75f),
                        190.0f};
    ImGui::SetNextWindowPos(
        ImVec2((m_settings.viewportSize.x - windowSize.x) / 2.0f,
               m_settings.viewportSize.y - windowSize.y - 100));
    ImGui::SetNextWindowSize(windowSize);
    m_settings.updateEquationEditorLayout = false;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, minWindowSize);
  ImGui::Begin("Equation Editor", nullptr,
               ImGuiWindowFlags_HorizontalScrollbar |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);
  windowSize = ImGui::GetWindowSize();

  static const std::size_t maxTextSize{80UL * 16};

  ImGui::Text("Injected code (GLSL ES 3.00):");
  ImGui::BeginChild("##code_child", ImVec2(0, windowSize.y - 134), true);
  ImGui::Text("Global scope:");
  ImGui::SameLine(windowSize.x / 2 - 4);
  ImGui::Text("Local scope:");

  std::string newExpression{};
  std::string newInjectedCodeGlobal{};
  std::string newInjectedCodeLocal{};

  // Global scope injected scope
  {
    std::array<char, maxTextSize + 1> injectedCode{};
    auto const &code{loadedData.codeGlobal};
    code.copy(injectedCode.data(), std::min(code.length(), maxTextSize));
    ImGui::PushFont(m_monospacedFont);
    ImGui::InputTextMultiline("##code_global_edit", injectedCode.data(),
                              injectedCode.size(),
                              ImVec2(windowSize.x / 2 - 20, windowSize.y - 170),
                              ImGuiInputTextFlags_AllowTabInput);
    ImGui::PopFont();
    newInjectedCodeGlobal.assign(injectedCode.data(), maxTextSize);
    newInjectedCodeGlobal.resize(newInjectedCodeGlobal.find('\0'));
  }

  // Local scope injected code
  {
    ImGui::SameLine();
    std::array<char, maxTextSize + 1> injectedCode{};
    auto const &code{loadedData.codeLocal};
    code.copy(injectedCode.data(), std::min(code.length(), maxTextSize));
    ImGui::PushFont(m_monospacedFont);
    ImGui::InputTextMultiline("##code_local_edit", injectedCode.data(),
                              injectedCode.size(),
                              ImVec2(windowSize.x / 2 - 20, windowSize.y - 170),
                              ImGuiInputTextFlags_AllowTabInput);
    ImGui::PopFont();
    newInjectedCodeLocal.assign(injectedCode.data(), maxTextSize);
    newInjectedCodeLocal.resize(newInjectedCodeLocal.find('\0'));
  }
  ImGui::EndChild();

  ImGui::Text("Equation:");
  auto const eqIsovalue{fmt::format("= {:.3g}", m_settings.isoValue)};
  auto const eqIsoValueSize{ImGui::CalcTextSize(eqIsovalue.c_str())};
  std::array<char, maxTextSize + 1> text{};
  auto const &expr{loadedData.expression};
#if defined(_MSC_VER)
  strncpy_s(text.data(), text.size(), expr.c_str(),
            std::min(maxTextSize, expr.length()));
#else
  strncpy(text.data(), expr.c_str(), std::min(maxTextSize, expr.length()));
#endif

  ImGui::PushFont(m_monospacedFont);
  ImGui::InputTextMultiline(
      "##eqn_edit", text.data(), text.size(),
      ImVec2(windowSize.x - (eqIsoValueSize.x + 25), inputTextHeight * 2 + 4),
      ImGuiInputTextFlags_AllowTabInput);
  ImGui::PopFont();

  newExpression.assign(text.data(), maxTextSize);
  newExpression.resize(newExpression.find('\0'));
  ImGui::SameLine();
  ImGui::AlignTextToFramePadding();
  ImGui::Text("%s", eqIsovalue.c_str());

  ImGui::End();
  ImGui::PopStyleVar();

  // Poll every second if the injected code or expression has changed
  static auto const pollInterval{1.0};
  static auto lastTime{ImGui::GetTime()};
  auto const currentTime{ImGui::GetTime()};
  if (currentTime - lastTime > pollInterval) {
    lastTime = currentTime;
    // Has anything changed?
    if (newExpression != loadedData.expression ||
        newInjectedCodeGlobal != loadedData.codeGlobal ||
        newInjectedCodeLocal != loadedData.codeLocal) {
      auto &oldEquation{m_equationGroups.at(m_settings.selectedUIGroupIndex)
                            .equations.at(m_settings.selectedUIEquationIndex)};

      // Create a user-defined equation from the current selected equation
      Equation::LoadedData oldLoadedData{oldEquation.getLoadedData()};
      oldLoadedData.name = "User-defined";
      oldLoadedData.expression = newExpression;
      oldLoadedData.codeGlobal = newInjectedCodeGlobal;
      oldLoadedData.codeLocal = newInjectedCodeLocal;
      oldLoadedData.comment = "";

      // Change selection to the user-defined equation
      m_settings.selectedUIGroupIndex = m_equationGroups.size() - 1;
      m_settings.selectedUIEquationIndex = 0;
      m_settings.rebuildProgram = true;

      Equation equation{oldLoadedData};
      auto &equations{
          m_equationGroups.at(m_settings.selectedUIGroupIndex).equations};
      equations.clear();
      equations.push_back(equation);

      m_settings.equation = equation;

#if defined(__EMSCRIPTEN__)
      updateEquationName(oldLoadedData.name);
      updateEquation(equation.getMathJaxExpression(m_settings.isoValue),
                     m_settings.overlayMathJaxComment ? oldLoadedData.comment
                                                      : "");
#endif
    }
  }
}

void Window::paintIsoValueWindow() {
  ImGui::SetNextWindowPos(ImVec2(5, m_settings.viewportSize.y - 44));
  ImGui::SetNextWindowSize(ImVec2(m_settings.viewportSize.x - 10, -1));
  ImGui::Begin("Isovalue Bar", nullptr, ImGuiWindowFlags_NoDecoration);

  static const auto defaultIsoMin{-2.0f};
  static const auto defaultIsoMax{2.0f};
  static auto isoMin{defaultIsoMin};
  static auto isoMax{defaultIsoMax};
  m_settings.isoValue = std::clamp(m_settings.isoValue, isoMin, isoMax);

  // Minimum iso value
  ImGui::PushItemWidth(50);
  ImGui::DragFloat("##iso_min", &isoMin, 0.1f, -1e5f, -0.1f, "%.1g");
  if (ImGui::IsItemHovered() && !ImGui::IsAnyMouseDown())
    ImGui::SetTooltip("Drag to change\nminimum value");
  ImGui::PopItemWidth();

  // Slider will fill the horizontal span of the window
  auto const widgetWidth{176};
  ImGui::SameLine();
  ImGui::PushItemWidth(m_settings.viewportSize.x - (widgetWidth + 22));
  ImGui::SliderFloat("##iso_slider", &m_settings.isoValue, isoMin, isoMax,
                     "Isovalue: %.3g", ImGuiSliderFlags_NoRoundToFormat);
  if (ImGui::IsItemHovered() && !ImGui::IsAnyMouseDown())
    ImGui::SetTooltip("Ctrl+click to\ninput value");
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Maximum iso value
  ImGui::PushItemWidth(50);
  ImGui::DragFloat("##iso_max", &isoMax, 0.1f, 0.1f, 1e5f, "%.1g");
  if (ImGui::IsItemHovered() && !ImGui::IsAnyMouseDown())
    ImGui::SetTooltip("Drag to change\nmaximum value");
  ImGui::PopItemWidth();

  // Reset button
  ImGui::SameLine();
  ImGui::PushItemWidth(50);
  if (ImGui::Button("Reset")) {
    isoMin = defaultIsoMin;
    isoMax = defaultIsoMax;
    m_settings.isoValue = 0.0f;
  }
  ImGui::PopItemWidth();
  ImGui::SameLine();

  ImGui::End();
}

void Window::paintParserDebugInfo() {
  if (m_settings.updateLogWindowLayout) {
    ImGui::SetNextWindowPos({5.0f, m_settings.viewportSize.y - 425});
    ImGui::SetNextWindowSize({m_settings.viewportSize.x / 2, 130});
    m_settings.updateLogWindowLayout = false;
  }

  ImGui::Begin("Parser Debug Info", nullptr,
               ImGuiWindowFlags_HorizontalScrollbar);
  {
    ImGui::PushFont(m_monospacedFont);

    auto const &loadedData{m_settings.equation.getLoadedData()};
    ImGui::Text("Original expression:\n%s", loadedData.expression.c_str());
    ImGui::Spacing();
    ImGui::Text(
        "MathJax:\n%s",
        m_settings.equation.getMathJaxExpression(m_settings.isoValue).c_str());
    ImGui::Spacing();
    ImGui::Text("GLSL:\n%s", m_settings.equation.getGLSLExpression().c_str());

    if (!m_settings.equation.getParameters().empty()) {
      std::string params;
      for (auto const &param : m_settings.equation.getParameters()) {
        params += fmt::format("name: {} value: {}\n", param.name, param.value);
      }

      ImGui::Spacing();
      ImGui::Text("Parameters:\n%s", params.c_str());
    }

    ImGui::PopFont();
  }
  ImGui::End();
}

void Window::resizeGL(int width, int height) {
  m_settings.viewportSize = {width, height};

  m_settings.updateEquationEditorLayout = true;
  m_settings.updateLogWindowLayout = true;

  if (m_backgroundRenderTex != 0U) {
    abcg::glDeleteTextures(1, &m_backgroundRenderTex);
  }
  if (m_implicitSurfaceRenderTex != 0U) {
    abcg::glDeleteTextures(1, &m_implicitSurfaceRenderTex);
  }

  // Initialize textures with an array of zeros to prevent WebGL warnings
  auto size{static_cast<unsigned long>(width * height * 4)};
  auto zeroBuffer{std::vector<std::byte>(size, std::byte{})};

  abcg::glGenTextures(1, &m_backgroundRenderTex);
  abcg::glBindTexture(GL_TEXTURE_2D, m_backgroundRenderTex);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  abcg::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, zeroBuffer.data());
  abcg::glBindTexture(GL_TEXTURE_2D, 0);
  m_settings.redrawBackgroundRenderTex = true;

  abcg::glGenTextures(1, &m_implicitSurfaceRenderTex);
  abcg::glBindTexture(GL_TEXTURE_2D, m_implicitSurfaceRenderTex);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  abcg::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, zeroBuffer.data());
  abcg::glBindTexture(GL_TEXTURE_2D, 0);

  m_background.resizeGL(width, height);
  m_textureBlit.resizeGL(width, height);
  m_rayCast.resizeGL(width, height);
}

void Window::terminateGL() {
  for (auto &buttonTexture : m_buttonTexture) {
    abcg::glDeleteTextures(1, &buttonTexture);
  }
  abcg::glDeleteTextures(1, &m_backgroundRenderTex);
  abcg::glDeleteTextures(1, &m_implicitSurfaceRenderTex);
  m_rayCast.terminateGL();
  m_textureBlit.terminateGL();
  m_background.terminateGL();

  for (auto &equationGroup : m_equationGroups) {
    for (auto &equation : equationGroup.equations) {
      equation.terminateGL();
    }
  }
}
