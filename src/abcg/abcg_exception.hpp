/**
 * @file abcg_exception.hpp
 * @brief Header file of abcg::Exception and derived classes.
 *
 * Declaration of abcg::Exception and derived classes.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_EXCEPTION_HPP_
#define ABCG_EXCEPTION_HPP_

#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
#if defined(__clang__)
#include <experimental/source_location>
namespace abcg {
using source_location = std::experimental::source_location;
} // namespace abcg
#else
#include <source_location>
namespace abcg {
using source_location = std::source_location;
} // namespace abcg
#endif
#endif

#include <stdexcept>
#include <string>

namespace abcg {
class Exception;
class RunTimeError;
class OpenGLError;
class SDLError;
class SDLImageError;
} // namespace abcg

/**
 * @brief Base class for ABCg exception objects.
 *
 * This is the base exception class used for exception objects thrown by ABCg.
 */
class abcg::Exception : public std::exception {
public:
  explicit Exception(std::string_view what);

  /**
   * @brief Returns the explanatory string.
   *
   * @return Pointer to a null-terminated string with explanatory information.
   */
  [[nodiscard]] const char *what() const noexcept override {
    return m_what.data();
  };

private:
  std::string m_what{};
};

/**
 * @brief Represents an exception object for runtime errors.
 *
 * This is used for throwing exceptions for errors not related to OpenGL, SDL,
 * or SDL_image.
 *
 * The explanatory error message is appended with source location information.
 */
class abcg::RunTimeError : public abcg::Exception {
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
public:
  explicit RunTimeError(
      std::string_view what,
      source_location const &sourceLocation = source_location::current());

private:
  [[nodiscard]] static std::string
  prettyPrint(std::string_view what, source_location const &sourceLocation);
#else
public:
  explicit RunTimeError(std::string_view what);

private:
  [[nodiscard]] static std::string prettyPrint(std::string_view what);
#endif
};

/**
 * @brief Represents an exception object for OpenGL errors.
 *
 * This is used for throwing exceptions for OpenGL errors that can be checked
 * with `glGetError`.
 *
 * The explanatory error message is appended with source location information,
 * and descriptive messages regarding the GL error codes.
 */
class abcg::OpenGLError : public abcg::Exception {
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
public:
  explicit OpenGLError(
      std::string_view what, unsigned int errorCode,
      source_location const &sourceLocation = source_location::current());

private:
  [[nodiscard]] static std::string
  prettyPrint(std::string_view what, unsigned int errorCode,
              source_location const &sourceLocation);
#else
public:
  explicit OpenGLError(std::string_view what, unsigned int errorCode);

private:
  [[nodiscard]] static std::string prettyPrint(std::string_view what,
                                               unsigned int errorCode);
#endif

  [[nodiscard]] static std::string_view
  getGLErrorString(unsigned int errorCode);
};

/**
 * @brief Represents an exception object for SDL errors.
 *
 * This is used for throwing exceptions for SDL errors that can be checked
 * with `SDL_GetError`.
 *
 * The explanatory error message is appended with source location information,
 * and the SDL error message.
 */
class abcg::SDLError : public abcg::Exception {
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
public:
  explicit SDLError(
      std::string_view what,
      source_location const &sourceLocation = source_location::current());

private:
  [[nodiscard]] static std::string
  prettyPrint(std::string_view what, source_location const &sourceLocation);
#else
public:
  explicit SDLError(std::string_view what);

private:
  [[nodiscard]] static std::string prettyPrint(std::string_view what);
#endif
};

/**
 * @brief Represents an exception object for SDL_image errors.
 *
 * This is used for throwing exceptions for SDL errors that can be checked
 * with `IMG_GetError`.
 *
 * The explanatory error message is appended with source location information,
 * and the SDL_image error message.
 */
class abcg::SDLImageError : public abcg::Exception {
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
public:
  explicit SDLImageError(
      std::string_view what,
      source_location const &sourceLocation = source_location::current());

private:
  [[nodiscard]] static std::string
  prettyPrint(std::string_view what, source_location const &sourceLocation);
#else
public:
  explicit SDLImageError(std::string_view what);

private:
  [[nodiscard]] static std::string prettyPrint(std::string_view what);
#endif
};

#endif
