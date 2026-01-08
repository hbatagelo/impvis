#version 300 es

/**
 * @file axes.frag
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

precision highp float;

in vec3 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;
in vec3 vLocalPos;

out vec4 outColor;

uniform vec3 uLightDirection;

uniform float uTickHalfWidth;
uniform float uCylinderHalfLength;
uniform float uCylinderRadius;
uniform float uLengthScale;
uniform float uRadiusScale;

void main()
{
  float axial = abs(vLocalPos.x);
  float radial = length(vLocalPos.yz);

  // Scale tick half width by radiusScale to maintain constant screen size
  float scaledTickHalfWidth = uTickHalfWidth * uRadiusScale;

  // Integer ticks (period = 1.0)
  float distInt = abs(fract(axial + 0.5) - 0.5);
  float dInt = distInt - scaledTickHalfWidth;
  float wInt = fwidth(dInt);
  float tickInt = smoothstep(wInt, 0.0, dInt);

  // Decimal ticks (period = 0.1)
  const float decimalScale = 10.0; // 1 / 0.1
  float axialDec = axial * decimalScale;
  // Distance to nearest decimal tick
  float distDec = abs(fract(axialDec + 0.5) - 0.5) / decimalScale;
  float dDec = distDec - scaledTickHalfWidth * 0.25;
  float wDec = fwidth(dDec);
  float tickDec = smoothstep(wDec, 0.0, dDec);
  // Suppress decimal ticks where integer ticks exist
  tickDec *= (1.0 - tickInt);

  // Radial + length masks
  // Use scaled cylinder radius for radial mask
  float scaledCylinderRadius = uCylinderRadius * uRadiusScale;
  float radialMask = smoothstep(scaledCylinderRadius * 1.05 + wInt,
                                scaledCylinderRadius * 1.05 - wInt,
                                radial);

  // Use scaled half length for length mask
  float scaledHalfLength = uCylinderHalfLength * uLengthScale;
  float lengthMask = smoothstep(scaledHalfLength + wInt,
                                scaledHalfLength - wInt,
                                axial);

  // Final masks
  tickInt *= radialMask * lengthMask;
  tickDec *= radialMask * lengthMask;

  const float ambient = 0.4;
  float diffuse = max(dot(normalize(fragNormal), -uLightDirection), 0.0) * 0.6;

  // Composition: integer ticks brighter than decimal ticks
  vec3 color = fragColor * (ambient + diffuse)
               + mix(vec3(0.0), vec3(1.0), tickInt)
               + mix(vec3(0.0), vec3(0.25), tickDec);

  outColor = vec4(color, 1.0);
}