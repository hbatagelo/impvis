#version 300 es

/**
 * @file glyph.frag
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

precision lowp float;

in vec2 fragTexCoord;
in float fragAlpha;
out vec4 outColor;

uniform sampler2D uFontTexture;
uniform vec3 uTextColor;

void main()
{
  float distance = texture(uFontTexture, fragTexCoord).r;

  float smoothing = fwidth(distance) * 0.5;
  float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);

  float outlineWidth = 0.2;
  float outlineAlpha = smoothstep(0.5 - outlineWidth - smoothing, 0.5 - outlineWidth + smoothing, distance);
  vec3 outlineColor = vec3(0.8);
  vec3 finalColor = mix(outlineColor, uTextColor, alpha);
  alpha = max(alpha, outlineAlpha * 0.8);

  alpha *= fragAlpha;

  // Premultiplied alpha
  outColor = vec4(finalColor * alpha, alpha);
}