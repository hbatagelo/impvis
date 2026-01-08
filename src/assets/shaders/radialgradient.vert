#version 300 es

/**
 * @file radialgradient.vert
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

layout(location = 0) in vec2 inPosition;

void main() {
  gl_Position = vec4(inPosition, 0.0, 1.0);
}