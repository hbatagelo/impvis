/**
 * @file arrow.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "arrow.hpp"

#include <abcgApplication.hpp>
#include <abcgException.hpp>
#include <abcgOpenGLFunction.hpp>
#include <abcgOpenGLShader.hpp>

#include <fstream>

namespace {

constexpr auto kConeLength{0.15f};
constexpr glm::vec3 kColor{1.0f, 0.8f, 0.2f}; // #ffcc33

} // namespace

void Arrow::onCreate() {
  auto const readFile{[](std::string_view filename) -> std::string {
    std::stringstream source;
    if (std::ifstream stream(filename.data()); stream) {
      source << stream.rdbuf();
      stream.close();
    } else {
      throw abcg::RuntimeError(
          std::format("Failed to read shader file {}", filename));
    }
    return source.str();
  }};

  static auto const &assetsPath{abcg::Application::getAssetsPath()};

  std::vector<abcg::ShaderSource> const sources{
      {.source = readFile(assetsPath + std::string{kVertexShaderPath}),
       .stage = abcg::ShaderStage::Vertex},
      {.source = readFile(assetsPath + std::string{kFragmentShaderPath}),
       .stage = abcg::ShaderStage::Fragment}};

  m_program = abcg::createOpenGLProgram(sources);

  // Get uniform locations
  m_viewMatrixLocation = abcg::glGetUniformLocation(m_program, "uViewMatrix");
  m_projMatrixLocation = abcg::glGetUniformLocation(m_program, "uProjMatrix");
  m_modelMatrixLocation = abcg::glGetUniformLocation(m_program, "uModelMatrix");
  m_normalMatrixLocation =
      abcg::glGetUniformLocation(m_program, "uNormalMatrix");
  m_lightDirectionLocation =
      abcg::glGetUniformLocation(m_program, "uLightDirection");
  m_arrowColorLocation = abcg::glGetUniformLocation(m_program, "uArrowColor");
  m_arrowModelMatrixLocation =
      abcg::glGetUniformLocation(m_program, "uArrowModelMatrix");
  m_radiusScaleLocation = abcg::glGetUniformLocation(m_program, "uRadiusScale");
  m_cylinderLengthLocation =
      abcg::glGetUniformLocation(m_program, "uCylinderLength");

  // Create arrow geometry (pointing along +Y axis)
  std::vector<Vertex> vertices;
  std::vector<GLuint> indices;

  auto const cylinderEnd{m_baseArrowLength};
  auto const coneStart{cylinderEnd};
  auto const coneEnd{cylinderEnd + kConeLength};
  auto const coneRadius{m_baseArrowRadius * 2.0f};

  // Cylinder from origin to cylinderEnd
  geometry::createCylinder(vertices, indices, {0, 0, 0}, {0, cylinderEnd, 0},
                           m_baseArrowRadius);

  // Cone from cylinderEnd to coneEnd
  geometry::createCone(vertices, indices, {0, coneStart, 0}, {0, coneEnd, 0},
                       coneRadius);

  m_numIndices = gsl::narrow<int>(indices.size());

  // Create VBO
  abcg::glGenBuffers(1, &m_VBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  abcg::glBufferData(GL_ARRAY_BUFFER,
                     gsl::narrow<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                     vertices.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Create EBO
  abcg::glGenBuffers(1, &m_EBO);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  abcg::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     gsl::narrow<GLsizeiptr>(indices.size() * sizeof(GLuint)),
                     indices.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Create VAO
  abcg::glGenVertexArrays(1, &m_VAO);
  abcg::glBindVertexArray(m_VAO);

  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

  // Position attribute
  abcg::glEnableVertexAttribArray(0);
  abcg::glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
      reinterpret_cast<void *>(offsetof(Vertex, position)));

  // Normal attribute
  abcg::glEnableVertexAttribArray(1);
  abcg::glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
      reinterpret_cast<void *>(offsetof(Vertex, normal)));

  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  abcg::glBindVertexArray(0);
}

void Arrow::onDestroy() {
  abcg::glDeleteProgram(m_program);
  abcg::glDeleteVertexArrays(1, &m_VAO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteBuffers(1, &m_EBO);
}

void Arrow::render(Camera const &camera) {
  if (!m_visible) {
    return;
  }

  auto const desiredWorldRadius{
      geometry::computeScreenSpaceRadius(camera, kTargetScreenRadius)};
  auto const radiusScale{desiredWorldRadius / m_baseArrowRadius};

  abcg::glUseProgram(m_program);

  auto const modelMatrix{camera.getModelMatrix()};
  auto const viewMatrix{camera.getViewMatrix()};
  auto const projMatrix{camera.getProjMatrix()};
  auto const normalMatrix{camera.getNormalMatrix()};

  abcg::glUniformMatrix4fv(m_viewMatrixLocation, 1, GL_FALSE,
                           &viewMatrix[0][0]);
  abcg::glUniformMatrix4fv(m_projMatrixLocation, 1, GL_FALSE,
                           &projMatrix[0][0]);
  abcg::glUniformMatrix4fv(m_modelMatrixLocation, 1, GL_FALSE,
                           &modelMatrix[0][0]);
  abcg::glUniformMatrix3fv(m_normalMatrixLocation, 1, GL_FALSE,
                           &normalMatrix[0][0]);
  abcg::glUniform1f(m_radiusScaleLocation, radiusScale);
  abcg::glUniform1f(m_cylinderLengthLocation, m_baseArrowLength);

  auto const lightDirectionView{glm::normalize(
      glm::vec3(viewMatrix * modelMatrix * glm::vec4(m_lightDirection, 0.0)))};
  abcg::glUniform3fv(m_lightDirectionLocation, 1, &lightDirectionView[0]);
  abcg::glUniform3fv(m_arrowColorLocation, 1, &kColor[0]);

  // Build transformation to align arrow with normal
  // Arrow is built along +Y, so we need to rotate from +Y to normal
  glm::vec3 up{0.0f, 1.0f, 0.0f};
  glm::mat4 arrowTransform{1.0f};

  // Translate to position
  arrowTransform = glm::translate(arrowTransform, m_position);

  // Rotate to align with normal
  if (glm::length(glm::cross(up, m_normal)) > 1e-6f) {
    auto const axis{glm::normalize(glm::cross(up, m_normal))};
    auto const angle{
        std::acos(glm::clamp(glm::dot(up, m_normal), -1.0f, 1.0f))};
    arrowTransform = glm::rotate(arrowTransform, angle, axis);
  } else if (glm::dot(up, m_normal) < 0.0f) {
    // Normal points down, rotate 180 degrees around X
    arrowTransform = glm::rotate(arrowTransform, glm::pi<float>(),
                                 glm::vec3(1.0f, 0.0f, 0.0f));
  }

  abcg::glUniformMatrix4fv(m_arrowModelMatrixLocation, 1, GL_FALSE,
                           &arrowTransform[0][0]);

  abcg::glEnable(GL_DEPTH_TEST);
  abcg::glDepthMask(GL_FALSE);
  abcg::glBindVertexArray(m_VAO);
  abcg::glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
  abcg::glBindVertexArray(0);
  abcg::glDepthMask(GL_TRUE);
  abcg::glDisable(GL_DEPTH_TEST);

  abcg::glUseProgram(0);
}