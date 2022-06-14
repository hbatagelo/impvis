/**
 * @file abcg_openglwindow.hpp
 * @brief Header file of abcg::OpenGLWindow.
 *
 * Declaration of abcg::OpenGLWindow.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_OPENGLWINDOW_HPP_
#define ABCG_OPENGLWINDOW_HPP_

#include <string>

#include "abcg_elapsedtimer.hpp"
#include "abcg_openglfunctions.hpp"
#include "abcg_shaders.hpp"

namespace abcg {
enum class OpenGLProfile;
class Application;
class OpenGLWindow;
struct OpenGLSettings;
struct WindowSettings;
#if defined(__EMSCRIPTEN__)
EM_BOOL fullscreenchangeCallback(int eventType,
                                 EmscriptenFullscreenChangeEvent const *event,
                                 void *userData);
#endif
} // namespace abcg

/**
 * @brief Enumeration of OpenGL profiles.
 *
 * @sa abcg::OpenGLSettings.
 */
enum class abcg::OpenGLProfile {
  /** @brief OpenGL core profile.
   *
   * Deprecated functions are disabled.
   */
  Core,
  /** @brief OpenGL compatibility profile.
   *
   * Deprecated functions are allowed.
   */
  Compatibility,
  /** @brief OpenGL ES profile.
   *
   * Only a subset of the base OpenGL functionality is available.
   */
  ES
};

/**
 * @brief Configuration settings for creating an OpenGL context.
 *
 * This structure contains the configuration settings to be used when creating
 * the OpenGL context. These must be set before calling
 * `abcg::Application::run`.
 *
 * @sa abcg::OpenGLWindow::getOpenGLSettings.
 * @sa abcg::OpenGLWindow::setOpenGLSettings.
 */
struct abcg::OpenGLSettings {
  /** @brief Type of OpenGL context. */
  OpenGLProfile profile{OpenGLProfile::Core};
  /** @brief OpenGL context major version. */
  int majorVersion{3};
  /** @brief OpenGL context minor version. */
  int minorVersion{3};
  /** @brief Minimum number of bits in the depth buffer. */
  int depthBufferSize{24};
  /** @brief Minimum number of bits in the stencil buffer. */
  int stencilBufferSize{0};
  /** @brief Number of samples used around the current pixel used for
   * multisample anti-aliasing. */
  int samples{0};
  /** @brief Whether the swapping of the front and back frame buffers is
   * synchronized with the vertical retrace. */
  bool verticalSync{false};
  /** @brief Whether the output is double buffered. */
  bool doubleBuffering{true};
};

/**
 * @brief Configuration settings of a window.
 *
 * This is a structure with configuration information related to the
 * application's window. The settings can be changed before and after window
 * creation.
 *
 * @sa abcg::OpenGLWindow::setWindowSettings.
 * @sa abcg::OpenGLWindow::getWindowSettings.
 */
struct abcg::WindowSettings {
  /** @brief Window width in pixels.
   *
   * This value may change when the window is resized.
   */
  int width{800};
  /** @brief Window height in pixels.
   *
   * This value may change when the window is resized.
   */
  int height{600};
  /** @brief Whether to show an overlay window with a FPS counter. */
  bool showFPS{true};
  /** @brief Whether to show a button to toggle fullscreen on/off. */
  bool showFullscreenButton{true};
  /** @brief HTML element ID used for registering the fullscreen callback when
   * the application is built for WebAssembly.
   */
  std::string fullscreenElementID{"#canvas"};
  /** @brief String containing the window title. */
  std::string title{"ABCg Window"};
};

/**
 * @brief Base class for a window that displays graphics using an OpenGL
 * context.
 *
 * Derive from this class to create a custom OpenGL window object to be passed
 * to abcg::Application::run.
 *
 * @sa abcg::OpenGLWindow::handleEvent for custom handling of SDL events.
 * @sa abcg::OpenGLWindow::initializeGL for custom initialization of OpenGL
 * resources.
 * @sa abcg::OpenGLWindow::paintGL for custom scene rendering commands.
 * @sa abcg::OpenGLWindow::paintUI for custom UI rendering commands.
 * @sa abcg::OpenGLWindow::resizeGL for custom handling of window resizing
 * events.
 * @sa abcg::OpenGLWindow::terminateGL for custom clean up of OpenGL resources.
 *
 * @remark Objects of this type cannot be copied or copy-constructed.
 */
class abcg::OpenGLWindow {
public:
  /**
   * @brief Default constructor.
   */
  OpenGLWindow() = default;
  OpenGLWindow(OpenGLWindow const &) = delete;
  /**
   * @brief Default move constructor.
   */
  OpenGLWindow(OpenGLWindow &&) = default;
  OpenGLWindow &operator=(OpenGLWindow const &) = delete;
  /**
   * @brief Default move constructor.
   */
  OpenGLWindow &operator=(OpenGLWindow &&) = default;
  /**
   * @brief Default destructor.
   */
  virtual ~OpenGLWindow() = default;

  [[nodiscard]] double getDeltaTime() const noexcept;
  [[nodiscard]] double getElapsedTime() const;
  [[nodiscard]] OpenGLSettings const &getOpenGLSettings() const noexcept;
  [[nodiscard]] WindowSettings const &getWindowSettings() const noexcept;
  void setOpenGLSettings(OpenGLSettings const &openGLSettings) noexcept;
  void setWindowSettings(WindowSettings const &windowSettings);

  void saveScreenshotPNG(std::string_view filename) const;
  void toggleFullscreen() const;

protected:
  virtual void handleEvent([[maybe_unused]] SDL_Event &event);
  virtual void initializeGL();
  virtual void paintGL();
  virtual void paintUI();
  virtual void resizeGL(int width, int height);
  virtual void terminateGL();

private:
  void handleEvent(SDL_Event &event, bool &done);
  void initialize();
  void cleanup() const;
  void paint();

  WindowSettings m_windowSettings;
  OpenGLSettings m_openGLSettings;

  std::string m_GLSLVersion;

  SDL_Window *m_window{};
  SDL_GLContext m_GLContext{};
  Uint32 m_windowID{};

  int m_viewportWidth{};
  int m_viewportHeight{};

  ElapsedTimer m_deltaTime;
  ElapsedTimer m_windowStartTime;
  double m_lastDeltaTime{};

  friend Application;

#if defined(__EMSCRIPTEN__)
  friend EM_BOOL
  fullscreenchangeCallback(int eventType,
                           EmscriptenFullscreenChangeEvent const *event,
                           void *userData);
#endif
};

#endif
