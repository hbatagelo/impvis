/**
 * @file abcgApplication.hpp
 * @brief Header file of abcg::Application.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2026 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_APPLICATION_HPP_
#define ABCG_APPLICATION_HPP_

#include <filesystem>

#define ABCG_VERSION_MAJOR 3
#define ABCG_VERSION_MINOR 1
#define ABCG_VERSION_PATCH 3

/**
 * @brief Root namespace.
 */
namespace abcg {
class Application;
class Window;
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

  void run(Window &window);

  static std::filesystem::path const &getAssetsPath() noexcept;
  static std::filesystem::path const &getBasePath() noexcept;

private:
  void mainLoopIterator(bool &done) const;

  Window *m_window{};

#if defined(__EMSCRIPTEN__)
  friend void mainLoopCallback(void *userData);
#endif

  static inline std::filesystem::path m_assetsPath;
  static inline std::filesystem::path m_basePath;
};

#endif
