/**
 * @file camera.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef CAMERA_HPP_
#define CAMERA_HPP_

#include <abcgTrackball.hpp>

class Camera {
public:
  enum Projection : std::uint8_t { Perspective, Orthographic };

  Camera();

  void handleEvent(SDL_Event const &event);
  void update();
  void resize(glm::ivec2 size);
  void setModelScale(float scale);
  void setProjection(Projection projection);
  void setFOV(float fov);

  [[nodiscard]] glm::vec3 getPosition() const noexcept { return m_position; }
  [[nodiscard]] glm::vec2 getPixelSize() const noexcept { return m_pixelSize; }
  [[nodiscard]] float getModelScale() const noexcept { return m_modelScale; }
  [[nodiscard]] float getFovY() const noexcept { return m_fovY; }
  [[nodiscard]] static float getLookAtDistance() noexcept {
    return kLookAtDistance;
  }
  [[nodiscard]] Projection getProjection() const noexcept {
    return m_projection;
  }
  [[nodiscard]] glm::mat4 const &getModelMatrix() const noexcept {
    return m_modelMatrix;
  }
  [[nodiscard]] glm::mat4 const &getInvModelMatrix() const noexcept {
    return m_invModelMatrix;
  }
  [[nodiscard]] glm::mat4 const &getViewMatrix() const noexcept {
    return m_viewMatrix;
  }
  [[nodiscard]] glm::mat4 const &getInvViewMatrix() const noexcept {
    return m_invViewMatrix;
  }
  [[nodiscard]] glm::mat4 const &getProjMatrix() const noexcept {
    return m_projMatrix;
  }
  [[nodiscard]] glm::mat4 const &getInvProjMatrix() const noexcept {
    return m_invProjMatrix;
  }
  [[nodiscard]] glm::mat3 const &getNormalMatrix() const noexcept {
    return m_normalMatrix;
  }

private:
  static constexpr float kLookAtDistance{10.0f};

  float m_aspectRatio{};
  Projection m_projection{Perspective};
  float m_fovY{30.0f};
  float m_modelScale{1.0f};
  glm::vec3 m_position{};
  glm::vec2 m_pixelSize{};
  glm::mat4 m_modelMatrix{};
  glm::mat4 m_invModelMatrix{};
  glm::mat4 m_viewMatrix{};
  glm::mat4 m_invViewMatrix{};
  glm::mat4 m_projMatrix{};
  glm::mat4 m_invProjMatrix{};
  glm::mat3 m_normalMatrix{};

  abcg::TrackBall m_trackBall;

  void rebuildModelMatrix();
  void rebuildViewMatrix();
  void rebuildProjMatrix();
  void rebuildNormalMatrix();
};

#endif
