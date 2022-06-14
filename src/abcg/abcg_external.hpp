/**
 * @file abcg_external.hpp
 * @brief Header file for including third-party dependencies.
 *
 * Preprocessor definitions and includes of third-party dependencies.
 *
 * This file is part of ABCg (https://github.com/hbatagelo/abcg).
 *
 * @copyright (c) 2021--2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT License.
 */

#ifndef ABCG_EXTERNAL_HPP_
#define ABCG_EXTERNAL_HPP_

// \cond (skipped by Doxygen)

#define GLM_ENABLE_EXPERIMENTAL

#if !defined(__EMSCRIPTEN__)
#if !defined(GLEW_STATIC)
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#endif

#include <SDL.h>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>

#include "SDL_opengles2.h"
#include "emscripten.h"
#include "emscripten/html5.h"
#endif

// \endcond

#endif
