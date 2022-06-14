/**
 * @file abcg_openglwindow.cpp
 * @brief Definition of abcg::OpenGLWindow members.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#include "abcg_openglwindow.hpp"

#include <SDL_image.h>
#include <cppitertools/itertools.hpp>
#include <fmt/core.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

#include <regex>

#include "SDL_events.h"
#include "SDL_video.h"
#include "abcg_embeddedfonts.hpp"
#include "abcg_exception.hpp"

static ImVec4 ColorAlpha(ImVec4 const &color, float const alpha) {
  return {color.x, color.y, color.z, alpha};
}

static void setupImGuiStyle(bool const darkTheme, float const alpha) {
  auto &style{ImGui::GetStyle()};

  ImVec4 const black{0.00f, 0.00f, 0.00f, 1.00f};
  ImVec4 const gray0{0.20f, 0.20f, 0.20f, 1.00f};
  ImVec4 const gray1{0.40f, 0.40f, 0.40f, 1.00f};
  ImVec4 const gray2{0.50f, 0.50f, 0.50f, 1.00f};
  ImVec4 const gray3{0.60f, 0.60f, 0.60f, 1.00f};
  ImVec4 const gray4{0.70f, 0.70f, 0.70f, 1.00f};
  ImVec4 const gray5{0.80f, 0.80f, 0.80f, 1.00f};
  ImVec4 const gray6{0.90f, 0.90f, 0.90f, 1.00f};
  ImVec4 const white{1.00f, 1.00f, 1.00f, 1.00f};

  // clang-format off
  style.Alpha                                 = 1.0f;
  style.FrameRounding                         = 4.0f;
  style.FrameBorderSize                       = 0.0f;
  style.WindowRounding                        = 5.0f;
  style.PopupRounding                         = 4.0f;
  style.GrabRounding                          = 3.0f;
  style.ChildRounding                         = 5.0f;
  style.ScrollbarSize                         = 15.0f;
  style.WindowTitleAlign                      = ImVec2(0.50f, 0.50f);
  style.Colors[ImGuiCol_Text]                 = ColorAlpha(black, 1.00f);
  style.Colors[ImGuiCol_TextDisabled]         = ColorAlpha(gray3, 1.00f);  
  style.Colors[ImGuiCol_WindowBg]             = ColorAlpha(gray5, 0.95f);
  style.Colors[ImGuiCol_ChildBg]              = ColorAlpha(white, 0.16f);
  style.Colors[ImGuiCol_PopupBg]              = ColorAlpha(gray6, 0.97f);
  style.Colors[ImGuiCol_Border]               = ColorAlpha(white, 0.20f);
  style.Colors[ImGuiCol_BorderShadow]         = ColorAlpha(white, 0.10f);
  style.Colors[ImGuiCol_FrameBg]              = ColorAlpha(gray6, 0.80f);
  style.Colors[ImGuiCol_FrameBgHovered]       = ColorAlpha(gray4, 1.00f);
  style.Colors[ImGuiCol_FrameBgActive]        = ColorAlpha(gray3, 0.67f);
  style.Colors[ImGuiCol_TitleBg]              = ColorAlpha(gray6, 0.95f);
  style.Colors[ImGuiCol_TitleBgCollapsed]     = ColorAlpha(gray4, 0.95f);
  style.Colors[ImGuiCol_TitleBgActive]        = ColorAlpha(gray4, 0.95f);
  style.Colors[ImGuiCol_MenuBarBg]            = ColorAlpha(gray6, 0.90f);
  style.Colors[ImGuiCol_ScrollbarBg]          = ColorAlpha(white, 0.00f);
  style.Colors[ImGuiCol_ScrollbarGrab]        = ColorAlpha(gray4, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = ColorAlpha(gray3, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabActive]  = ColorAlpha(gray2, 1.00f);
  style.Colors[ImGuiCol_CheckMark]            = ColorAlpha(gray0, 1.00f);
  style.Colors[ImGuiCol_SliderGrab]           = ColorAlpha(gray1, 0.95f);
  style.Colors[ImGuiCol_SliderGrabActive]     = ColorAlpha(gray1, 1.00f);
  style.Colors[ImGuiCol_Button]               = ColorAlpha(gray1, 0.30f);
  style.Colors[ImGuiCol_ButtonHovered]        = ColorAlpha(gray2, 0.80f);
  style.Colors[ImGuiCol_ButtonActive]         = ColorAlpha(gray2, 0.90f);
  style.Colors[ImGuiCol_Header]               = ColorAlpha(gray4, 0.67f); // 0.50
  style.Colors[ImGuiCol_HeaderHovered]        = ColorAlpha(gray3, 0.95f); // 0.80
  style.Colors[ImGuiCol_HeaderActive]         = ColorAlpha(gray3, 1.00f);
  style.Colors[ImGuiCol_ResizeGrip]           = ColorAlpha(white, 0.50f);
  style.Colors[ImGuiCol_ResizeGripHovered]    = ColorAlpha(gray2, 0.67f);
  style.Colors[ImGuiCol_ResizeGripActive]     = ColorAlpha(gray2, 0.95f);
  style.Colors[ImGuiCol_PlotLines]            = ColorAlpha(gray1, 1.00f);
  style.Colors[ImGuiCol_PlotLinesHovered]     = ColorAlpha(gray0, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram]        = ColorAlpha(gray1, 1.00f);
  style.Colors[ImGuiCol_PlotHistogramHovered] = ColorAlpha(gray0, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg]       = ColorAlpha(gray4, 0.67f);
  style.Colors[ImGuiCol_ModalWindowDimBg]     = ColorAlpha(gray0, 0.35f);
  style.Colors[ImGuiCol_NavHighlight]         = ColorAlpha(gray1, 1.00f);
  style.Colors[ImGuiCol_Tab]                  = ColorAlpha(gray1, 0.30f);
  style.Colors[ImGuiCol_TabHovered]           = ColorAlpha(gray2, 0.95f);
  style.Colors[ImGuiCol_TabActive]            = ColorAlpha(gray2, 0.90f);
  style.Colors[ImGuiCol_TabUnfocused]         = ColorAlpha(gray6, 0.80f);
  style.Colors[ImGuiCol_TabUnfocusedActive]   = ColorAlpha(gray5, 1.00f);
  style.Colors[ImGuiCol_DragDropTarget]       = ColorAlpha(black, 1.00f);
  style.Colors[ImGuiCol_Separator]            = ColorAlpha(gray2, 0.50f);
  style.Colors[ImGuiCol_SeparatorHovered]     = ColorAlpha(gray2, 0.67f);
  style.Colors[ImGuiCol_SeparatorActive]      = ColorAlpha(gray2, 0.95f);
  // clang-format on

  if (darkTheme) {
    for (ImVec4 &col : style.Colors) {
      auto hue{0.0f};
      auto sat{0.0f};
      auto val{0.0f};
      ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, hue, sat, val);

      if (sat < 0.1f) {
        val = 1.0f - val;
      }
      ImGui::ColorConvertHSVtoRGB(hue, sat, val, col.x, col.y, col.z);
      if (col.w < 1.00f) {
        col.w *= alpha;
      }
    }
  } else {
    for (ImVec4 &col : style.Colors) {
      if (col.w < 1.00f) {
        col.x *= alpha;
        col.y *= alpha;
        col.z *= alpha;
        col.w *= alpha;
      }
    }
  }
}

#if defined(__EMSCRIPTEN__)
EM_BOOL
abcg::fullscreenchangeCallback(int eventType,
                               EmscriptenFullscreenChangeEvent const *event,
                               void *userData) {
  abcg::OpenGLWindow &window{*(static_cast<abcg::OpenGLWindow *>(userData))};
  if (event->isFullscreen) {
    SDL_SetWindowSize(window.m_window, event->screenWidth, event->screenHeight);
  }
  return true;
}
#endif

/**
 * @brief Returns the configuration settings of the OpenGL context.
 *
 * @returns Reference to the abcg::OpenGLSettings structure used for creating
 * the OpenGL context.
 */
abcg::OpenGLSettings const &
abcg::OpenGLWindow::getOpenGLSettings() const noexcept {
  return m_openGLSettings;
}

/**
 * @brief Returns the current configuration settings of the window.
 *
 * @returns Reference to the abcg::OpenGLWindow structure used by the window.
 */
abcg::WindowSettings const &
abcg::OpenGLWindow::getWindowSettings() const noexcept {
  return m_windowSettings;
}

/**
 * @brief Sets the configuration settings that will be used for creating the
 * OpenGL context.
 *
 * This function will have no effect if called after the creation of the
 * OpenGL context.
 */
void abcg::OpenGLWindow::setOpenGLSettings(
    OpenGLSettings const &openGLSettings) noexcept {
  if (m_window != nullptr)
    return;
  m_openGLSettings = openGLSettings;
}

/**
 * @brief Sets the configuration settings of the window.
 */
void abcg::OpenGLWindow::setWindowSettings(
    WindowSettings const &windowSettings) {
  if (m_window != nullptr) {
    if (windowSettings.title != m_windowSettings.title) {
      SDL_SetWindowTitle(m_window, windowSettings.title.c_str());
    }

    if (windowSettings.width != m_windowSettings.width ||
        windowSettings.height != m_windowSettings.height) {
      SDL_SetWindowSize(m_window, windowSettings.width, windowSettings.height);
    }

#if defined(__EMSCRIPTEN__)
    if (windowSettings.fullscreenElementID !=
        m_windowSettings.fullscreenElementID) {
      emscripten_set_fullscreenchange_callback(
          windowSettings.fullscreenElementID.c_str(), this, true,
          fullscreenchangeCallback);
    }
#endif
  }

  m_windowSettings = windowSettings;
}

/**
 * @brief Custom event handler.
 *
 * This virtual function is called whenever there is a pending event polled by
 * SDL.
 *
 * Override it for custom behavior. By default, it does nothing.
 *
 * @param event Event structure containing information about the pending event.
 */
void abcg::OpenGLWindow::handleEvent([[maybe_unused]] SDL_Event &event) {}

/**
 * @brief Custom handler for OpenGL initialization tasks to be performed before
 * rendering the scene.
 *
 * This virtual function is called once after the OpenGL context is created.
 *
 * Override it for custom behavior. By default, it only calls `glClearColor`
 * with the RGBA color (0, 0, 0, 1).
 */
void abcg::OpenGLWindow::initializeGL() { glClearColor(0, 0, 0, 1); }

/**
 * @brief Custom handler for rendering the OpenGL scene.
 *
 * This virtual function is called for each frame in the rendering loop, just
 * after abcg::OpenGLWindow::paintUI is called.
 *
 * Override it for custom behavior. By default, it only clears the color buffer.
 */
void abcg::OpenGLWindow::paintGL() { glClear(GL_COLOR_BUFFER_BIT); }

/**
 * @brief Custom handler for rendering Dear ImGUI controls.
 *
 * This virtual function is called for each frame in the rendering loop, just
 * before abcg::OpenGLWindow::paintGL is called.
 *
 * Override it for custom behavior. By default, it shows a FPS counter if
 * abcg::WindowSettings::showFPS is set to `true`, and a toggle fullscren button
 * if abcg::WindowSettings::showFullscreenButton is set to `true`.
 */
void abcg::OpenGLWindow::paintUI() {
  // FPS counter
  if (m_windowSettings.showFPS) {
    auto fps{ImGui::GetIO().Framerate};

    static auto offset{0UL};
    static auto refreshTime{ImGui::GetTime()};
    static std::array<float, 150> frames{};

    while (refreshTime < ImGui::GetTime()) {
      constexpr auto refreshFrequency{60.0};
      frames.at(offset) = fps;
      offset = (offset + 1) % frames.size();
      refreshTime += (1.0 / refreshFrequency);
    }

    ImGui::SetNextWindowPos(ImVec2(5, 5));
    ImGui::Begin("FPS", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoFocusOnAppearing);
    auto const label{fmt::format("avg {:.1f} FPS", fps)};
    ImGui::PlotLines("", frames.data(), static_cast<int>(frames.size()),
                     static_cast<int>(offset), label.c_str(), 0.0f,
                     // *std::ranges::max_element(frames) * 2,
                     *std::max_element(frames.begin(), frames.end()) * 2,
                     ImVec2(static_cast<float>(frames.size()), 50));
    ImGui::End();
  }

  // Fullscreen button
  if (m_windowSettings.showFullscreenButton) {
#if defined(__EMSCRIPTEN__)
    auto const isFullscreenAvailable{
        static_cast<bool>(EM_ASM_INT({ return document.fullscreenEnabled; })) &&
        static_cast<bool>(EM_ASM_INT({ return !isMobile(); }))};
    if (isFullscreenAvailable)
#endif
    {
      auto windowWidth{0};
      auto windowHeight{0};
      SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);

      auto const widgetSize{ImVec2(150.0f, 30.0f)};
      auto const windowBorder{ImVec2(16.0f, 16.0f)};

      ImGui::SetNextWindowSize(
          ImVec2(widgetSize.x + windowBorder.x, widgetSize.y + windowBorder.y));
      ImGui::SetNextWindowPos(ImVec2(5, static_cast<float>(windowHeight) -
                                            (widgetSize.y + windowBorder.y) -
                                            5));

      constexpr auto windowFlags{ImGuiWindowFlags_NoDecoration |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoFocusOnAppearing};
      ImGui::Begin("Fullscreen", nullptr, windowFlags);

      if (ImGui::Button("Toggle fullscreen", widgetSize)) {
        toggleFullscreen();
      }

      ImGui::End();
    }
  }
}

/**
 * @brief Custom handler for window resizing events.
 *
 * This virtual function is called whenever the window is resized.
 *
 * @param width New window width, in pixels.
 * @param height New window height, in pixels.
 *
 * Override it for custom behavior. By default, it calls `glViewport` with
 * arguments (0, 0, \p width, \p height).
 */
void abcg::OpenGLWindow::resizeGL(int const width, int const height) {
  glViewport(0, 0, width, height);
}

/**
 * @brief Custom handler for cleaning up OpenGL resources.
 *
 * This virtual function is called once when the application is exiting.
 *
 * Override it for custom behavior. By default, it does nothing.
 */
void abcg::OpenGLWindow::terminateGL() {}

/**
 * @brief Returns the time that have passed since the last frame.
 *
 * If the time is smaller than 2ms, the function returns 0. In that case
 * the inner timer is not restarted and the delta time accumulates for the
 * next frame(s) until at least 2ms have passed.
 *
 * @returns Time in seconds.
 */
double abcg::OpenGLWindow::getDeltaTime() const noexcept {
  return m_lastDeltaTime;
}

/**
 * @brief Returns the time that have passed since the window has been created.
 *
 * @returns Time in seconds.
 */
double abcg::OpenGLWindow::getElapsedTime() const {
  return m_windowStartTime.elapsed();
}

/**
 * @brief Toggles on/off fullscreen.
 */
void abcg::OpenGLWindow::toggleFullscreen() const {
#if defined(__EMSCRIPTEN__)
  EM_ASM(toggleFullscreen(););
#else
  constexpr Uint32 windowFlags{SDL_WINDOW_FULLSCREEN |
                               SDL_WINDOW_FULLSCREEN_DESKTOP};
  bool const fullscreen{(SDL_GetWindowFlags(m_window) & windowFlags) != 0u};
  enum class WindowType { Windowed, Fullscreen, FullscreenWindow };

  switch (auto const desiredWindowType{fullscreen ? WindowType::Windowed
                                                  : WindowType::Fullscreen};
          desiredWindowType) {
  case WindowType::Windowed:
    SDL_SetWindowFullscreen(m_window, 0);
    SDL_SetWindowSize(m_window, m_windowSettings.width,
                      m_windowSettings.height);
    break;
  case WindowType::FullscreenWindow:
    SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);
    break;
  case WindowType::Fullscreen:
    SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    break;
  }
  SDL_ShowCursor(static_cast<int>(fullscreen));
#endif
}

void abcg::OpenGLWindow::handleEvent(SDL_Event &event, bool &done) {
  ImGui_ImplSDL2_ProcessEvent(&event);

  if (event.window.windowID != m_windowID)
    return;

  if (event.type == SDL_WINDOWEVENT) {
    switch (event.window.event) {
    case SDL_WINDOWEVENT_CLOSE:
      done = true;
      break;
    case SDL_WINDOWEVENT_SIZE_CHANGED: {
      auto const &newWidth{event.window.data1};
      auto const &newHeight{event.window.data2};
      if (newWidth >= 0 && newHeight >= 0 &&
          (newWidth != m_viewportWidth || newHeight != m_viewportHeight)) {
        m_viewportWidth = newWidth;
        m_viewportHeight = newHeight;
        resizeGL(newWidth, newHeight);
      }
    } break;
    case SDL_WINDOWEVENT_RESIZED: {
      if (auto const fullscreen{
              (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0u};
          !fullscreen) {
        m_windowSettings.width = event.window.data1;
        m_windowSettings.height = event.window.data2;
      }
#if defined(__EMSCRIPTEN__)
      m_windowSettings.width = event.window.data1;
      m_windowSettings.height = event.window.data2;
      SDL_SetWindowSize(m_window, m_windowSettings.width,
                        m_windowSettings.height);
#endif
      m_viewportWidth = event.window.data1;
      m_viewportHeight = event.window.data2;
      resizeGL(event.window.data1, event.window.data2);
    } break;
    default:
      break;
    }
  }
  if (event.type == SDL_KEYUP) {
    if (event.key.keysym.sym == SDLK_F11) {
#if defined(__EMSCRIPTEN__)
      auto isFullscreenAvailable{
          static_cast<bool>(
              EM_ASM_INT({ return document.fullscreenEnabled; })) &&
          static_cast<bool>(EM_ASM_INT({ return !isMobile(); }))};
      if (isFullscreenAvailable)
#endif
        toggleFullscreen();
    }
  }

  // Won't pass mouse events to the application if ImGUI has captured the
  // mouse
  auto useCustomEventHandler{true};
  if (ImGui::GetIO().WantCaptureMouse &&
      (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN ||
       event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL)) {
    useCustomEventHandler = false;
  }

  // Won't pass keyboard events to the application if ImGUI has captured the
  // keyboard
  if (ImGui::GetIO().WantCaptureKeyboard &&
      (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP ||
       event.type == SDL_TEXTEDITING || event.type == SDL_TEXTINPUT ||
       event.type == SDL_KEYMAPCHANGED)) {
    useCustomEventHandler = false;
  }

  if (useCustomEventHandler)
    handleEvent(event);
}

void abcg::OpenGLWindow::initialize() {
  m_deltaTime.restart();
  m_windowStartTime.restart();

#if defined(__EMSCRIPTEN__)
  if (!m_openGLSettings.doubleBuffering) {
    emscripten_run_script(
        "canvas.getContext('webgl2', {preserveDrawingBuffer : true});\n"
        "canvas.style.backgroundColor = '#000000';");
  }
#endif

  // Shortcuts
  auto &majorVersion{m_openGLSettings.majorVersion};
  auto &minorVersion{m_openGLSettings.minorVersion};
  auto &profile{m_openGLSettings.profile};

#if defined(__EMSCRIPTEN__)
  profile = OpenGLProfile::ES; // WebGL 2.0
#elif defined(__APPLE__)
  profile = OpenGLProfile::Core;
  majorVersion = std::min(majorVersion, 4);
  if (majorVersion == 4)
    minorVersion = 1;
#endif

  if (profile == OpenGLProfile::ES) {
    majorVersion = 3;
#if defined(__EMSCRIPTEN__)
    minorVersion = 0;
#else
    minorVersion = std::min(std::max(minorVersion, 0), 2);
#endif
  } else {
    // Constrain to the range [3.3, 4.0, 4.1, 4.2, 4.3, 4.4, 4.5, 4.6]
    majorVersion = std::min(std::max(majorVersion, 3), 4);
    if (majorVersion == 3) {
      minorVersion = 3;
    } else {
      minorVersion = std::min(std::max(minorVersion, 0), 6);
    }
  }

  m_GLSLVersion =
      fmt::format("#version {:d}{:02d}", majorVersion, minorVersion * 10);

  switch (profile) {
  case OpenGLProfile::Core:
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    m_GLSLVersion += " core";
    break;
  case OpenGLProfile::Compatibility:
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    m_GLSLVersion += " compatibility";
    break;
  case OpenGLProfile::ES:
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    m_GLSLVersion += " es";
    break;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,
                      m_openGLSettings.doubleBuffering ? 1 : 0);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, m_openGLSettings.depthBufferSize);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, m_openGLSettings.stencilBufferSize);

  if (m_openGLSettings.samples > 0) {
    // Enable multisampling
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // Can be 2, 4, 8 or 16
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, m_openGLSettings.samples);
  } else {
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
  }

  // Create window with graphics context
  while (true) {
    m_window = SDL_CreateWindow(m_windowSettings.title.c_str(),
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                m_windowSettings.width, m_windowSettings.height,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr && m_openGLSettings.samples > 0) {
      // Try again, but this time with multisampling disabled
      m_openGLSettings.samples = 0;
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
      fmt::print("Warning: multisampling requested but not supported!\n");
    } else {
      break;
    }
  };

  if (m_window == nullptr) {
    throw abcg::SDLError("SDL_CreateWindow failed");
  }

  m_windowID = SDL_GetWindowID(m_window);

#if defined(__EMSCRIPTEN__)
  emscripten_set_fullscreenchange_callback(
      m_windowSettings.fullscreenElementID.c_str(), this, true,
      fullscreenchangeCallback);
#endif

  // Create OpenGL context
  m_GLContext = SDL_GL_CreateContext(m_window);
  if (m_GLContext == nullptr) {
    throw abcg::SDLError("SDL_GL_CreateContext failed");
  }

#if !defined(__EMSCRIPTEN__)
  SDL_GL_SetSwapInterval(m_openGLSettings.verticalSync ? 1 : 0);
#endif

  auto const toCharPtr{
      [](GLubyte const *str) { return reinterpret_cast<char const *>(str); }};

#if !defined(__EMSCRIPTEN__)
  if (auto const err{glewInit()}; GLEW_OK != err) {
    std::string const header{"Failed to initialize OpenGL loader: "};
    auto const *const message{toCharPtr(glewGetErrorString(err))};
    throw abcg::Exception{header + message};
  }
  fmt::print("Using GLEW.....: {}\n", toCharPtr(glewGetString(GLEW_VERSION)));
#endif

  fmt::print("OpenGL vendor..: {}\n", toCharPtr(glGetString(GL_VENDOR)));
  fmt::print("OpenGL renderer: {}\n", toCharPtr(glGetString(GL_RENDERER)));
  fmt::print("OpenGL version.: {}\n", toCharPtr(glGetString(GL_VERSION)));
  fmt::print("GLSL version...: {}\n",
             toCharPtr(glGetString(GL_SHADING_LANGUAGE_VERSION)));

  /*
  // Print out extensions
  GLint numExtensions{};
  glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
  for (std::size_t posStart{}; auto const index : iter::range(numExtensions)) {
    std::string name{
        reinterpret_cast<char const *>(glGetStringi(GL_EXTENSIONS, index))};
    fmt::print("GL extension {}: {}\n", index, name);
  }
  */

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &guiIO{ImGui::GetIO()};
  // Enable keyboard controls
  guiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // Enable gamepad controls
  guiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  // For an Emscripten build we are disabling file-system access, so let's
  // not attempt to do a fopen() of the imgui.ini file. You may manually
  // call LoadIniSettingsFromMemory() to load settings from your own
  // storage.
  guiIO.IniFilename = nullptr;

  // Set up our own Dear ImGui style
  setupImGuiStyle(true, 1.0f);

  // Setup platform/renderer bindings
  ImGui_ImplSDL2_InitForOpenGL(m_window, m_GLContext);
  ImGui_ImplOpenGL3_Init(m_GLSLVersion.c_str());

  // Load fonts
  guiIO.Fonts->Clear();

  ImFontConfig fontConfig;
  fontConfig.FontDataOwnedByAtlas = false;
  if (std::array ttf{INCONSOLATA_MEDIUM_TTF};
      guiIO.Fonts->AddFontFromMemoryTTF(ttf.data(),
                                        static_cast<int>(ttf.size()), 16.0f,
                                        &fontConfig) == nullptr) {
    throw abcg::RunTimeError("Failed to load font file");
  }

  initializeGL();

  if (guiIO.DisplaySize.x >= 0 && guiIO.DisplaySize.y >= 0) {
    auto const width{static_cast<int>(guiIO.DisplaySize.x)};
    auto const height{static_cast<int>(guiIO.DisplaySize.y)};
    m_viewportWidth = width;
    m_viewportHeight = height;
    resizeGL(width, height);
  } else {
    m_viewportWidth = m_windowSettings.width;
    m_viewportHeight = m_windowSettings.height;
    resizeGL(m_windowSettings.width, m_windowSettings.height);
  }
}

void abcg::OpenGLWindow::paint() {
  SDL_GL_MakeCurrent(m_window, m_GLContext);

#if defined(__EMSCRIPTEN__)
  // Force window size in windowed mode
  EmscriptenFullscreenChangeEvent fullscreenStatus{};
  emscripten_get_fullscreen_status(&fullscreenStatus);
  if (fullscreenStatus.isFullscreen == EM_FALSE) {
    SDL_SetWindowSize(m_window, m_windowSettings.width,
                      m_windowSettings.height);
  }
#endif

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  paintUI();
  ImGui::Render();
  paintGL();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  if (m_openGLSettings.doubleBuffering) {
    SDL_GL_SwapWindow(m_window);
  } else {
    glFinish();
  }

  // Cap to 480 Hz
  if (m_deltaTime.elapsed() >= 1.0 / 480.0) {
    m_lastDeltaTime = m_deltaTime.restart();
  } else {
    m_lastDeltaTime = 0.0;
  }
}

/**
 * @brief Takes a snapshot of the screen and saves it to a file.
 *
 * @param filename String view to the filename.
 */
void abcg::OpenGLWindow::saveScreenshotPNG(std::string_view filename) const {
  auto const width{m_viewportWidth};
  auto const height{m_viewportHeight};
  auto const bitsPerPixel{8};
  auto const channels{4};
  auto const pitch{static_cast<long>(width * channels)};

  auto const numPixels{static_cast<std::size_t>(width * height * channels)};
  std::vector<unsigned char> pixels(numPixels);
  glReadBuffer(m_openGLSettings.doubleBuffering ? GL_BACK : GL_FRONT);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  // Flip upside down
  for (auto const line : iter::range(height / 2)) {
    std::swap_ranges(pixels.begin() + pitch * line,
                     pixels.begin() + pitch * (line + 1),
                     pixels.begin() + pitch * (height - line - 1));
  }

  if (auto *const surface{SDL_CreateRGBSurfaceFrom(
          pixels.data(), width, height, channels * bitsPerPixel,
          static_cast<int>(pitch), 0x000000FF, 0x0000FF00, 0x00FF0000,
          0xFF000000)}) {
    IMG_SavePNG(surface, filename.data());
    SDL_FreeSurface(surface);
  }
}

void abcg::OpenGLWindow::cleanup() const {
  if (m_window != nullptr) {
    if (ImGui::GetCurrentContext() != nullptr) {
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplSDL2_Shutdown();
      ImGui::DestroyContext();
    }
    if (m_GLContext != nullptr) {
      SDL_GL_DeleteContext(m_GLContext);
    }
    SDL_DestroyWindow(m_window);
  }
}