#version 300 es

layout(location = 0) in vec2 inPosition;

out vec2 fragPosition;

void main() {
  fragPosition = inPosition;
  gl_Position = vec4(inPosition, 0, 1);
}