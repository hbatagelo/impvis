/**
 * @file arrow.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef ARROW_HPP_
#define ARROW_HPP_

#include "camera.hpp"
#include "geometry.hpp"

class Arrow {
public:
  void onCreate();
  void onDestroy();

  void render(Camera const &camera);

  void setVisible(bool visible) noexcept { m_visible = visible; }
  void setPosition(glm::vec3 position) noexcept { m_position = position; }
  void setNormal(glm::vec3 normal) noexcept {
    m_normal = glm::normalize(normal);
  }
  void setLightDirection(glm::vec3 direction) noexcept {
    m_lightDirection = direction;
  }

private:
  // Desired world-space radius (0.3% of screen height)
  static constexpr auto kTargetScreenRadius{0.003f};

  static constexpr std::string_view kVertexShaderPath{"shaders/arrow.vert"};
  static constexpr std::string_view kFragmentShaderPath{"shaders/arrow.frag"};

  using Vertex = geometry::Vertex;

  GLuint m_program{};
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_EBO{};

  GLint m_viewMatrixLocation{};
  GLint m_projMatrixLocation{};
  GLint m_modelMatrixLocation{};
  GLint m_normalMatrixLocation{};
  GLint m_lightDirectionLocation{};
  GLint m_arrowColorLocation{};
  GLint m_arrowModelMatrixLocation{};
  GLint m_radiusScaleLocation{};
  GLint m_cylinderLengthLocation{};

  int m_numIndices{};
  bool m_visible{false};
  glm::vec3 m_position{0.0f};
  glm::vec3 m_normal{0.0f, 1.0f, 0.0f};
  float m_baseArrowLength{0.1f};
  float m_baseArrowRadius{0.025f};
  glm::vec3 m_lightDirection{0.0f, -1.0f, 0.0f};
};

#endif