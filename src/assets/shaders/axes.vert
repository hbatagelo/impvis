#version 300 es

/**
 * @file axes.vert
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

layout(location = 0) in vec3 inPosition; // Local coords: x = axial, yz = radial
layout(location = 1) in vec3 inNormal;

uniform mat4 uViewMatrix;
uniform mat4 uProjMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;

uniform mat4 uInstanceModelMatrix[3];
uniform vec3 uInstanceColor[3];

uniform float uRadiusScale;
uniform float uLengthScale;
uniform float uCylinderHalfLength;

out vec3 fragColor;
out vec3 fragNormal;
out vec3 fragPosition;
out vec3 vLocalPos;

void main()
{
  vec3 scaledPosition = inPosition;
  vec3 scaledNormal = inNormal;

  float cylinderEnd = uCylinderHalfLength;
  bool isCone = inPosition.x >= cylinderEnd - 0.001; // Cone starts at cylinderEnd

  if (isCone)
  {
    // For cone: scale uniformly with radiusScale to maintain proportions
    // First scale the cylinder end position
    float scaledCylinderEnd = cylinderEnd * uLengthScale;

    // Cone base is at the scaled cylinder end
    vec3 coneBase = vec3(scaledCylinderEnd, 0.0, 0.0);
    vec3 offsetFromBase = inPosition - vec3(cylinderEnd, 0.0, 0.0);

    // Scale uniformly with radiusScale
    offsetFromBase *= uRadiusScale;
    scaledPosition = coneBase + offsetFromBase;

    // Scale the normal uniformly
    scaledNormal = normalize(scaledNormal);
  } else {
    // Cylinder: scale axially and radially
    scaledPosition.x *= uLengthScale;
    scaledPosition.yz *= uRadiusScale;

    // Transform normal (scale radial component)
    if (length(inNormal.yz) > 0.01)
    {
      scaledNormal.yz *= uRadiusScale;
      scaledNormal = normalize(scaledNormal);
    }
  }

  // Save scaled local coords for tick mark computation
  // For cylinder part, use scaled position; for cone, clamp to cylinder end
  if (isCone) {
    vLocalPos = vec3(cylinderEnd * uLengthScale, scaledPosition.yz);
  } else {
    vLocalPos = scaledPosition;
  }

  // Apply per-instance model then global model/view/proj
  mat4 instanceModelMatrix = uInstanceModelMatrix[gl_InstanceID];
  vec4 worldPos = uModelMatrix * instanceModelMatrix * vec4(scaledPosition, 1.0);
  fragPosition = worldPos.xyz;

  // Transform normal
  mat3 instanceNormalMatrix = mat3(instanceModelMatrix);
  fragNormal = uNormalMatrix * (instanceNormalMatrix * scaledNormal);

  // Use per-instance color
  fragColor = uInstanceColor[gl_InstanceID];

  gl_Position = uProjMatrix * uViewMatrix * worldPos;
}
