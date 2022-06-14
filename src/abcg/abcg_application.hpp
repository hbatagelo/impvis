/**
 * @file abcg_application.hpp
 * @brief Header file of abcg::Application.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_APPLICATION_HPP_
#define ABCG_APPLICATION_HPP_

#include <string>

/**
 * @brief Root namespace.
 */
namespace abcg {
class Application;
class OpenGLWindow;
#if defined(__EMSCRIPTEN__)
void mainLoopCallback(void *userData);
#endif
} // namespace abcg

/**
 * @brief Manages the application's control flow.
 *
 * This is the class that starts an ABCg application, initializes the SDL
 * modules and enters the main event loop.
 */
class abcg::Application {
public:
  Application(int argc, char **argv);

  void run(OpenGLWindow &window);

  /**
   * @brief Returns the path to the application's assets directory, relative to
   * the directory the executable is launched in.
   *
   * @return Path to the application's `assets` directory, relative to the
   * directory the executable is launched. For example, the assets path is
   * `./app/assets/` if the application is located in `./app` and is launched
   * from its parent directory. The assets path is `./assets/` if the
   * application is launched from the same directory of the executable.
   *
   * @remark The assets path is appended with the base path (see
   * abcg::Application::getBasePath).
   * @remark The assetss path always ends with a forward slash.
   */
  [[nodiscard]] static std::string const &getAssetsPath() {
    return m_assetsPath;
  }

  /**
   * @brief Returns the path to the application's directory, relative to the
   * directory the executable is launched in.
   *
   * @return Path to the directory that contains the application executable,
   * relative to the directory the executable is launched. For example, the base
   * path is `./app` if the executable is located in `./app` and is called from
   * the parent directory. The base path is `.` if the application is launched
   * from the same directory of the executable.
   *
   * @remark The returned path does not ends with a slash.
   */
  [[nodiscard]] static std::string const &getBasePath() { return m_basePath; }

private:
  void mainLoopIterator(bool &done) const;

  OpenGLWindow *m_window{};

#if defined(__EMSCRIPTEN__)
  friend void mainLoopCallback(void *userData);
#endif

  // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
  // See https://bugs.llvm.org/show_bug.cgi?id=48040
  static std::string m_assetsPath;
  static std::string m_basePath;
  // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
};

#endif
