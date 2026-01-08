#version 300 es

/**
 * @file arrow.vert
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

uniform mat4 uViewMatrix;
uniform mat4 uProjMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;
uniform mat4 uArrowModelMatrix;

uniform float uRadiusScale;
uniform float uCylinderLength;

out vec3 fragNormal;
out vec3 fragPosition;

void main()
{
  vec3 scaledPosition = inPosition;
  vec3 scaledNormal = inNormal;

  bool isCone = inPosition.y >= uCylinderLength - 0.001;

  if (isCone)
  {
    // Scale uniformly to maintain cone shape
    // Translate to cone base, scale, translate back
    vec3 coneBase = vec3(0.0, uCylinderLength, 0.0);
    vec3 offsetFromBase = inPosition - coneBase;
    offsetFromBase *= uRadiusScale;
    scaledPosition = coneBase + offsetFromBase;

    // Scale the normal uniformly too
    scaledNormal = inNormal;
    if (length(inNormal.xz) > 0.01)
    {
      scaledNormal = normalize(scaledNormal);
    }
  } else {
    // Only scale radial components (x, z)
    scaledPosition.xz *= uRadiusScale;

    // Transform normal (scale radial component)
    if (length(inNormal.xz) > 0.01)
    {
      scaledNormal.xz *= uRadiusScale;
      scaledNormal = normalize(scaledNormal);
    }
  }

  // Apply arrow transformation, then model/view/proj
  vec4 worldPos = uModelMatrix * uArrowModelMatrix * vec4(scaledPosition, 1.0);
  fragPosition = worldPos.xyz;

  // Transform normal
  mat3 arrowNormalMat = mat3(uArrowModelMatrix);
  fragNormal = uNormalMatrix * (arrowNormalMat * scaledNormal);

  gl_Position = uProjMatrix * uViewMatrix * worldPos;
}