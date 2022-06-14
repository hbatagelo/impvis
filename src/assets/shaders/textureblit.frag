#version 300 es

precision lowp float;

out vec4 outColor;

uniform vec2 uResolution;
uniform sampler2D uTexture;

void main() { 
  vec2 normalizedFragCoord = gl_FragCoord.xy / vec2(uResolution);
  outColor = texture(uTexture, normalizedFragCoord);
}