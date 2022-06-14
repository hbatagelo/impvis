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

#include "abcg.hpp"
#include "settings.hpp"

class RayCast {
public:
  void handleEvent(SDL_Event &event);
  void initializeGL(Settings const &settings);
  void paintGL(Settings const &settings, GLuint renderTexture = 0);
  void resizeGL(int width, int height);
  void terminateGL();

  [[nodiscard]] bool isProgramValid() const noexcept { return m_program > 0; }
  void setLookAtDistance(float distance) noexcept {
    m_lastLookAtDistance = m_lookAtDistance;
    m_lookAtDistance = distance;
  }

  [[nodiscard]] glm::vec4 getKaIa() const noexcept { return m_shading.KaIa; }
  void setKaIa(glm::vec4 KaIa) { m_shading.KaIa = KaIa; };

private:
  char const *m_vertexShaderPath{"shaders/raycast.vert"};
  char const *m_fragmentShaderPath{"shaders/raycast.frag"};

  struct Vertex {
    glm::vec2 position{};
  };

  struct alignas(16) Camera {
    glm::vec3 eye{0.0f};
    float focalLength{4.0f};
    glm::vec2 scale{1.0f};
    float lookAtDistance{0.0f};

    std::array<unsigned char, 4> padding{};
  };

  struct alignas(16) Shading {
    glm::vec4 KaIa{0.1f, 0.1f, 0.25f, 1.0f};
    glm::vec4 KdId{0.9f, 0.2f, 0.01f, 1.0f};
    glm::vec4 KsIs{1.0f};
    glm::vec3 lightVector{1.0f};
    float shininess{100.0f};
    float gaussianEps{0.75f};

    std::array<unsigned char, 12> padding{};
  };

  struct alignas(16) Transform {
    glm::mat4 MV_I{1.0f};
  };

  struct alignas(16) Params {
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
  abcg::ElapsedTimer m_programBuildTime;
  std::vector<GLuint> m_shaderIDs{};
  GLuint m_nextProgram{};
  bool m_throwOnBuild{};

  void createProgram(Settings const &settings);
  void createUBOs();
  void createVBOs();
  void setupVAO();
};

#endif
