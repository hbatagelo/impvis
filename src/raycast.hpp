/**
 * @file raycast.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#ifndef RAYCAST_HPP_
#define RAYCAST_HPP_

#include "settings.hpp"

class RayCast {
public:
  void onEvent(SDL_Event const &event);
  void onCreate(Settings const &settings);
  void onPaint(Settings const &settings, GLuint renderTexture = 0);
  void onResize(glm::ivec2 const &size);
  void onDestroy();

  [[nodiscard]] bool isProgramValid() const noexcept { return m_program > 0; }
  void setLookAtDistance(float distance) noexcept {
    m_lastLookAtDistance = m_lookAtDistance;
    m_lookAtDistance = distance;
  }

  [[nodiscard]] glm::vec4 getKaIa() const noexcept { return m_shading.KaIa; }
  void setKaIa(glm::vec4 KaIa) { m_shading.KaIa = KaIa; };

  [[nodiscard]] bool buildFailed() const noexcept { return m_buildFailed; }

private:
  static inline char const *const m_vertexShaderPath{"shaders/raycast.vert"};
  static inline char const *const m_fragmentShaderPath{"shaders/raycast.frag"};

  struct Vertex {
    glm::vec2 position{};
  };

  struct Camera {
    glm::vec3 eye{0.0f};
    float focalLength{4.0f};

    glm::vec2 scale{1.0f};
    alignas(8) float lookAtDistance{0.0f};
  };

  struct Shading {
    glm::vec4 KaIa{0.1f, 0.1f, 0.25f, 1.0f};
    glm::vec4 KdId{0.9f, 0.2f, 0.01f, 1.0f};
    glm::vec4 KsIs{1.0f};

    glm::vec3 lightVector{1.0f};
    float shininess{100.0f};

    alignas(16) float gaussianEps{0.75f};
  };

  struct Transform {
    glm::mat4 MV_I{1.0f};
  };

  struct Params {
    std::array<glm::vec4, 4> data;
  };

  Camera m_camera;
  Shading m_shading;
  Transform m_transform;
  Params m_params;

  GLuint m_FBO{};
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_program{};
  GLuint m_UBOCamera{};
  GLuint m_UBOShading{};
  GLuint m_UBOTransform{};
  GLuint m_UBOParams{};
  GLint m_isoValueLocation{};

  float m_lookAtDistance{10.0f};
  float m_lastLookAtDistance{};
  abcg::TrackBall m_trackBallCamera;
  abcg::TrackBall m_trackBallLight;

  enum class ProgramBuildPhase { Compile, Link, Done };
  ProgramBuildPhase m_programBuildPhase{ProgramBuildPhase::Done};
  abcg::Timer m_programBuildTime;
  std::vector<abcg::OpenGLShader> m_shaderIDs{};
  GLuint m_nextProgram{};
  bool m_throwOnBuild{};
  bool m_buildFailed{};

  void createProgram(Settings const &settings);
  void createUBOs();
  void destroyUBOs();
  void createVBOs();
  void setupVAO();
};

#endif
