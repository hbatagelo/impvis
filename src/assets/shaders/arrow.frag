#version 300 es

/**
 * @file arrow.frag
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

precision lowp float;

in vec3 fragNormal;
in vec3 fragPosition;

out vec4 outColor;

uniform vec3 uLightDirection;
uniform vec3 uArrowColor;

void main()
{
  float ambient = 0.4;
  float diffuse = max(dot(normalize(fragNormal), -uLightDirection), 0.0) * 0.6;
  vec3 color = uArrowColor * (ambient + diffuse);

  outColor = vec4(color, 1.0);
}