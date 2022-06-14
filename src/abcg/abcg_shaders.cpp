/**
 * @file abcg_shaders.cpp
 * @brief Definition of shader building helper functions.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#include "abcg_shaders.hpp"
#include "abcg_application.hpp"

#include <cppitertools/itertools.hpp>
#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "abcg_exception.hpp"

static void printShaderInfoLog(GLuint const shader, std::string_view prefix) {
  GLint infoLogLength{};
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

  if (infoLogLength > 0) {
    std::vector<GLchar> infoLog{};
    infoLog.reserve(static_cast<std::size_t>(infoLogLength));
    glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());
    fmt::print("Shader information log ({} shader):\n{}\n", prefix,
               infoLog.data());
  }
}

static void printProgramInfoLog(GLuint const program) {
  GLint infoLogLength{};
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

  if (infoLogLength > 0) {
    std::vector<GLchar> infoLog{};
    infoLog.reserve(static_cast<std::size_t>(infoLogLength));
    glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.data());
    fmt::print("Program information log:\n{}\n", infoLog.data());
  }
}

static char const *shaderTypeToText(GLuint type) {
  switch (type) {
  case GL_VERTEX_SHADER:
    return "vertex";
  case GL_FRAGMENT_SHADER:
    return "fragment";
#if !defined(__EMSCRIPTEN__)
  case GL_GEOMETRY_SHADER:
    return "geometry";
  case GL_TESS_CONTROL_SHADER:
    return "tess control";
  case GL_TESS_EVALUATION_SHADER:
    return "tess evaluation";
  case GL_COMPUTE_SHADER:
    return "compute";
#endif
  default:
    return "unknown";
  }
};

/**
 * @brief Creates a program object from a group of shader paths or source codes.
 *
 * @param shaders Path or source code of the shaders to be compiled and linked
 * to the program.
 * @param throwOnError Whether to throw an exception on compile/link errors. If
 * this if `false`, the function will not throw exceptions and will return
 * silently on errors.
 *
 * @throw abcg::RunTimeError if the shader could not be read from file, or the
 * program could not be created, or the compilation of any shader failed, or the
 * linking failed.
 *
 * @return ID of the program object, or 0 on error.
 */
GLuint abcg::opengl::createProgram(Shaders const &shaders, bool throwOnError) {
  // Reads from str if it is a filename, otherwise returns str
  auto toShaderSource{[](std::string_view str) {
    if (!std::filesystem::exists(str))
      return std::string{str};
    std::stringstream shaderSource;
    if (std::ifstream stream(str.data()); stream) {
      shaderSource << stream.rdbuf();
      stream.close();
    } else {
      throw abcg::RunTimeError(
          fmt::format("Failed to read shader file {}", str));
    }
    return shaderSource.str();
  }};

  Shaders sources{};
  sources.vertexShader = toShaderSource(shaders.vertexShader);
  sources.fragmentShader = toShaderSource(shaders.fragmentShader);
  sources.geometryShader = toShaderSource(shaders.geometryShader);
  sources.tessControlShader = toShaderSource(shaders.tessControlShader);
  sources.tessEvalShader = toShaderSource(shaders.tessEvalShader);
  sources.computeShader = toShaderSource(shaders.computeShader);

  std::vector<GLuint> shaderIDs;

  auto deleteShaders{[&shaderIDs]() {
    for (auto const &shaderID : shaderIDs) {
      glDeleteShader(shaderID);
    }
  }};

  // Compile shader
  auto compileHelper{[&](std::string const &shaderSource, GLuint shaderType) {
    if (shaderSource.empty())
      return true;
    auto shaderID{glCreateShader(shaderType)};
    auto const *source{shaderSource.c_str()};
    glShaderSource(shaderID, 1, &source, nullptr);
    glCompileShader(shaderID);
    GLint compileStatus{};
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
      if (throwOnError) {
        fmt::print("\n");
        printShaderInfoLog(shaderID, shaderTypeToText(shaderType));
        deleteShaders();
        throw abcg::RunTimeError(fmt::format("Failed to compile {} shader",
                                             shaderTypeToText(shaderType)));
      }
      deleteShaders();
      return false;
    }
    shaderIDs.push_back(shaderID);
    return true;
  }};

  if (!compileHelper(sources.vertexShader, GL_VERTEX_SHADER))
    return 0U;
  if (!compileHelper(sources.fragmentShader, GL_FRAGMENT_SHADER))
    return 0U;
#if !defined(__EMSCRIPTEN__)
  if (!compileHelper(sources.geometryShader, GL_GEOMETRY_SHADER))
    return 0U;
  if (!compileHelper(sources.tessControlShader, GL_TESS_CONTROL_SHADER))
    return 0U;
  if (!compileHelper(sources.tessEvalShader, GL_TESS_EVALUATION_SHADER))
    return 0U;
  if (!compileHelper(sources.computeShader, GL_COMPUTE_SHADER))
    return 0U;
#endif

  auto const shaderProgram{glCreateProgram()};
  if (shaderProgram == 0) {
    deleteShaders();
    if (throwOnError) {
      throw abcg::RunTimeError("Failed to create program");
    }
    return 0;
  }

  for (auto const &shaderID : shaderIDs) {
    glAttachShader(shaderProgram, shaderID);
  }

  glLinkProgram(shaderProgram);

  for (auto const &shaderID : shaderIDs) {
    glDetachShader(shaderProgram, shaderID);
    glDeleteShader(shaderID);
  }

  GLint linkStatus{};
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
  if (linkStatus == GL_FALSE) {
    if (throwOnError) {
      fmt::print("\n");
      printProgramInfoLog(shaderProgram);
      glDeleteProgram(shaderProgram);
      throw abcg::RunTimeError("Failed to link program");
    }
    glDeleteProgram(shaderProgram);
    return 0U;
  }

  return shaderProgram;
}

/**
 * @brief Triggers the compilation of a group of shaders and returns
 * immediately.
 *
 * This function immediately returns the shader objects without querying the
 * compile status. The compilation may still be running in parallel. The compile
 * status must be checked later with abcg::opengl::checkCompile.
 *
 * This is an alternative to abcg::opengl::createProgram to prevent halting the
 * application when building complex programs. Instead of waiting for completion
 * of the build process, the user user can call abcg::opengl::triggerCompile,
 * followed by abcg::opengl::checkCompile, abcg::opengl::triggerLink, and
 * abcg::opengl::checkLink, interleaved with other processing tasks.
 *
 * @param shaders Path or source code of the shaders to be compiled.
 *
 * @throw abcg::RunTimeError if the shader could not be read from file.
 *
 * @return Container of shader objects begin compiled.
 *
 * @sa abcg::opengl::checkCompile.
 */
std::vector<GLuint> abcg::opengl::triggerCompile(Shaders const &shaders) {
  // Reads from str if it is a filename, otherwise returns str
  auto toShaderSource{[](std::string_view str) {
    std::error_code errorCode{};
    if (!std::filesystem::exists(str, errorCode))
      return std::string{str};
    std::stringstream shaderSource;
    if (std::ifstream stream(str.data()); stream) {
      shaderSource << stream.rdbuf();
      stream.close();
    } else {
      throw abcg::RunTimeError(
          fmt::format("Failed to read shader file {}", str));
    }
    return shaderSource.str();
  }};

  Shaders sources{};
  sources.vertexShader = toShaderSource(shaders.vertexShader);
  sources.fragmentShader = toShaderSource(shaders.fragmentShader);
  sources.geometryShader = toShaderSource(shaders.geometryShader);
  sources.tessControlShader = toShaderSource(shaders.tessControlShader);
  sources.tessEvalShader = toShaderSource(shaders.tessEvalShader);
  sources.computeShader = toShaderSource(shaders.computeShader);

  std::vector<GLuint> shaderIDs;

  // Compile shader but don't wait until completion
  auto compileHelper{[&](std::string const &shaderSource, GLuint shaderType) {
    GLuint shaderID{0};
    if (!shaderSource.empty()) {
      shaderID = glCreateShader(shaderType);
      auto const *source{shaderSource.c_str()};
      glShaderSource(shaderID, 1, &source, nullptr);
      glCompileShader(shaderID);
    }
    shaderIDs.push_back(shaderID);
  }};

  compileHelper(sources.vertexShader, GL_VERTEX_SHADER);
  compileHelper(sources.fragmentShader, GL_FRAGMENT_SHADER);
#if !defined(__EMSCRIPTEN__)
  compileHelper(sources.geometryShader, GL_GEOMETRY_SHADER);
  compileHelper(sources.tessControlShader, GL_TESS_CONTROL_SHADER);
  compileHelper(sources.tessEvalShader, GL_TESS_EVALUATION_SHADER);
  compileHelper(sources.computeShader, GL_COMPUTE_SHADER);
#endif

  return shaderIDs;
}

/**
 * @brief Queries the compile status of shader objects.
 *
 * This should be called after abcg::opengl::triggerCompile. The function will
 * wait until all shaders are compiled.
 *
 * @param shaderIDs Shader objects returned by abcg::opengl::triggerCompile.
 * @param throwOnError Whether to throw an exception on error. If
 * this if `false`, the function will return silently on error.
 *
 * @throw abcg::RunTimeError if the compilation of any shader failed.
 *
 * @return `true` if the shaders were compiled with success; `false` otherwise.
 *
 * @sa abcg::opengl::triggerLink.
 */
bool abcg::opengl::checkCompile(std::vector<GLuint> const &shaderIDs,
                                bool throwOnError) {
  auto deleteShaders{[&shaderIDs]() {
    for (auto const &shaderID : shaderIDs) {
      if (shaderID == 0)
        continue;
      glDeleteShader(shaderID);
    }
  }};

  // Check compile status
  for (auto &&[index, shader] : iter::enumerate(shaderIDs)) {
    if (shader == 0) // Ignore if shader was not compiled
      continue;
    GLint compileStatus{};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
      if (throwOnError) {
#if !defined(__EMSCRIPTEN__)
        static std::array const shaderTypes{
            GL_VERTEX_SHADER,          GL_FRAGMENT_SHADER,
            GL_GEOMETRY_SHADER,        GL_TESS_CONTROL_SHADER,
            GL_TESS_EVALUATION_SHADER, GL_COMPUTE_SHADER};
#else
        static std::array const shaderTypes{GL_VERTEX_SHADER,
                                            GL_FRAGMENT_SHADER};
#endif
        fmt::print("\n");
        auto const type{static_cast<GLuint>(shaderTypes.at(index))};
        printShaderInfoLog(shader, shaderTypeToText(type));
        deleteShaders();
        throw abcg::RunTimeError(
            fmt::format("Failed to compile {} shader", shaderTypeToText(type)));
      }
      deleteShaders();
      return false;
    }
  }
  return true;
}

/**
 * @brief Triggers the linking of a group of shader objects.
 *
 * This should be called after abcg::opengl::checkCompile. The function will
 * return immediately.
 *
 * @param shaderIDs Shader objects returned by abcg::opengl::triggerCompile.
 * @param throwOnError Whether to throw an exception on error. If
 * this if `false`, the function will return silently on error.
 *
 * @throw abcg::RunTimeError if the program could not be created.
 *
 * @return ID of the program object with the shaders attached, or 0 on error.
 *
 * @sa abcg::opengl::checkLink.
 */
GLuint abcg::opengl::triggerLink(std::vector<GLuint> const &shaderIDs,
                                 bool throwOnError) {
  auto deleteShaders{[&shaderIDs]() {
    for (auto const &shaderID : shaderIDs) {
      if (shaderID == 0)
        continue;
      glDeleteShader(shaderID);
    }
  }};

  auto const shaderProgram{glCreateProgram()};
  if (shaderProgram == 0) {
    deleteShaders();
    if (throwOnError) {
      throw abcg::RunTimeError("Failed to create program");
    }
    return 0;
  }

  for (auto const &shaderID : shaderIDs) {
    if (shaderID == 0)
      continue;
    glAttachShader(shaderProgram, shaderID);
  }

  glLinkProgram(shaderProgram);

  for (auto const &shaderID : shaderIDs) {
    if (shaderID == 0)
      continue;
    glDetachShader(shaderProgram, shaderID);
  }

  deleteShaders();

  return shaderProgram;
}

/**
 * @brief Queries the link status of the shaders attached to a program object.
 *
 * This should be called after abcg::opengl::triggerLink. The function will
 * wait until all shaders are linked.
 *
 * @param shaderProgram ID of the shader program returned by
 * abcg::opengl::triggerLink.
 * @param throwOnError Whether to throw an exception on error. If
 * this if `false`, the function will return silently on error.
 *
 * @throw abcg::RunTimeError if program linking failed.
 *
 * @return `true` if the shaders were linked with success; `false` otherwise.
 */
bool abcg::opengl::checkLink(GLuint shaderProgram, bool throwOnError) {
  GLint linkStatus{};
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
  if (linkStatus == GL_FALSE) {
    if (throwOnError) {
      fmt::print("\n");
      printProgramInfoLog(shaderProgram);
      glDeleteProgram(shaderProgram);
      throw abcg::RunTimeError("Failed to link program");
    }
    glDeleteProgram(shaderProgram);
    return false;
  }

  return true;
}