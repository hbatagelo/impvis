/**
 * @file camera.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "camera.hpp"

Camera::Camera() {
  m_trackBall.setAxis(glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)));
  m_trackBall.setVelocity(1e-4f);
  rebuildModelMatrix();
  rebuildViewMatrix();
}

void Camera::handleEvent(SDL_Event const &event) {
  glm::vec2 mousePosition;
  SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    m_trackBall.mouseMove(mousePosition);
  }
  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    if (event.button.button == SDL_BUTTON_LEFT) {
      m_trackBall.mousePress(mousePosition);
    }
  }
  if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
    if (event.button.button == SDL_BUTTON_LEFT) {
      m_trackBall.mouseRelease(mousePosition);
    }
  }
  if (event.type == SDL_EVENT_MOUSE_WHEEL) {
    auto const zoomStrength{0.05f};
    m_modelScale *= std::exp(event.wheel.y * zoomStrength);
    rebuildModelMatrix();
  }
}

void Camera::update() {
  static glm::quat trackBallRotation{};
  if (auto const rotation{m_trackBall.getRotation()};
      rotation != trackBallRotation) {
    trackBallRotation = rotation;
    rebuildViewMatrix();
  }
}

void Camera::resize(glm::ivec2 size) {
  Expects(size.x > 0 && size.y > 0);

  m_aspectRatio = gsl::narrow<float>(size.x) / gsl::narrow<float>(size.y);
  m_pixelSize = glm::vec2(2.0f / gsl::narrow<float>(size.x),
                          2.0f / gsl::narrow<float>(size.y));

  m_trackBall.resizeViewport(size);
  rebuildProjMatrix();
}

void Camera::setModelScale(float scale) {
  if (scale != m_modelScale) {
    m_modelScale = scale;
    rebuildModelMatrix();
  }
}

void Camera::setProjection(Projection projection) {
  if (projection != m_projection) {
    m_projection = projection;
    rebuildProjMatrix();
  }
}
void Camera::setFOV(float fov) {
  if (fov != m_fovY) {
    m_fovY = fov;
    rebuildProjMatrix();
  }
}

void Camera::rebuildModelMatrix() {
  m_modelMatrix = glm::scale(glm::mat4{1.0f}, glm::vec3(m_modelScale));
  m_invModelMatrix = glm::inverse(m_modelMatrix);
  rebuildNormalMatrix();
}

void Camera::rebuildViewMatrix() {
  auto const rotation{glm::inverse(m_trackBall.getRotation())};
  m_position = rotation * glm::vec3{0.0f, 0.0f, kLookAtDistance};
  auto const at{glm::vec3{0.0f}};
  auto const up{rotation * glm::vec3{0.0f, 1.0f, 0.0f}};

  m_viewMatrix = glm::lookAt(m_position, at, up);
  m_invViewMatrix = glm::inverse(m_viewMatrix);
  rebuildNormalMatrix();
}

void Camera::rebuildNormalMatrix() {
  m_normalMatrix = glm::inverseTranspose(m_viewMatrix * m_modelMatrix);
}

void Camera::rebuildProjMatrix() {
  auto const near{0.01f};
  auto const far{100.0f};

  if (m_aspectRatio <= 0.0f) {
    m_projMatrix = glm::mat4{1.0f};
    m_invProjMatrix = glm::mat4{1.0f};
    return;
  }

  if (m_projection == Perspective) {
    m_projMatrix =
        glm::perspective(glm::radians(m_fovY), m_aspectRatio, near, far);
  } else {
    // Match apparent scale between perspective and ortho views
    float orthoHeight =
        2.0f * kLookAtDistance * std::tan(glm::radians(m_fovY) * 0.5f);
    m_projMatrix = glm::ortho(-m_aspectRatio * orthoHeight / 2,
                              m_aspectRatio * orthoHeight / 2, -orthoHeight / 2,
                              orthoHeight / 2, near, far);
  }

  m_invProjMatrix = glm::inverse(m_projMatrix);
}
