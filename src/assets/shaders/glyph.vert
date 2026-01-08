#version 300 es

/**
 * @file glyph.vert
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

layout(location = 0) in vec2 inTexCoord;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjMatrix;
uniform float uAspectRatio;
uniform float uBillboardScale;
uniform vec3 uBillboardPosition;
uniform int uFadeAlpha;
uniform float uBoundsRadius;
uniform float uCameraDistanceToOrigin;
uniform vec2 uGlyphUV0;
uniform vec2 uGlyphUV1;

out vec2 fragTexCoord;
out float fragAlpha;

void main()
{
  vec3 right = vec3(uViewMatrix[0][0], uViewMatrix[1][0], uViewMatrix[2][0]);
  vec3 up = vec3(uViewMatrix[0][1], uViewMatrix[1][1], uViewMatrix[2][1]);

  vec3 worldPos = uBillboardPosition +
                  (inTexCoord.x - 0.5) * right * uBillboardScale * uAspectRatio +
                  (inTexCoord.y - 0.5) * up * uBillboardScale;
  vec4 viewPos = uViewMatrix * uModelMatrix * vec4(worldPos, 1.0);

  gl_Position = uProjMatrix * viewPos;

  fragTexCoord = mix(uGlyphUV0, uGlyphUV1, inTexCoord);

  float d = viewPos.z + uCameraDistanceToOrigin;
  // Fade from 0 -> 1 when d goes from -radius -> 0
  fragAlpha = mix(1.0, smoothstep(-uBoundsRadius, 0.0, d), float(uFadeAlpha));
}