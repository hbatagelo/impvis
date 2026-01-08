#version 300 es

/**
 * @file blit.frag
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

precision lowp float;

in vec2 fragTexCoord;

out vec4 outColor;

uniform sampler2D uColorTexture;
uniform vec4 uTintColor;

void main() {
  vec2 texCoord = fragTexCoord;

  outColor = texture(uColorTexture, texCoord) * uTintColor;
}