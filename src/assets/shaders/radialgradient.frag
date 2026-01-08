#version 300 es

/**
 * @file radialgradient.frag
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

precision lowp float;

out vec4 outColor;

uniform vec2 uResolution;

// Radial gradient with hard-coded focal point and color stops
vec3 radialGradient() {
  struct Stop { float at; vec3 color; };

  const vec2 focalPoint = vec2(-0.05, 0.025); // Normalized device coordinates
  const Stop stops[] = Stop[](
    Stop(0.0, vec3(105.0/255.0, 146.0/255.0, 182.0/255.0)),
    Stop(0.4, vec3( 81.0/255.0, 113.0/255.0, 150.0/255.0)),
    Stop(0.9, vec3( 16.0/255.0,  56.0/255.0, 121.0/255.0)));

  vec2 st = ((gl_FragCoord.xy - uResolution / 2.0) /
            max(uResolution.x, uResolution.y)) * 2.0;

  const float max_d = length(vec2(1.0));
  float d = distance(focalPoint, st) / max_d;

  for (int i = 1; i < stops.length(); ++i) {
    Stop x = stops[i - 1];
    Stop y = stops[i];
    if (d < y.at) {
      float t = (d - x.at) / (y.at - x.at);
      return mix(x.color, y.color, t);
    }
  }
  return stops[stops.length()-1].color;
}

void main() {
  outColor = vec4(radialGradient(), 1.0);
}
