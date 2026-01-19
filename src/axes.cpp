/**
 * @file axes.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "axes.hpp"

#include <abcgOpenGL.hpp>
#include <abcgOpenGLError.hpp>

#include <fstream>

namespace {

constexpr std::array<glm::vec3, 3> kInstanceColors{
    glm::vec3{0.929f, 0.333f, 0.392f}, // #ed5564
    glm::vec3{0.627f, 0.835f, 0.408f}, // #a0d568
    glm::vec3{0.098f, 0.510f, 0.769f}  // #1982c4
};

} // namespace

void Axes::onCreate() {
  auto const readFile{[](std::filesystem::path const &path) -> std::string {
    auto const utf8Path{abcg::pathToUtf8(path)};
    std::stringstream source;
    if (std::ifstream stream(utf8Path); stream) {
      source << stream.rdbuf();
      stream.close();
    } else {
      throw abcg::RuntimeError(
          std::format("Failed to read shader file {}", utf8Path));
    }
    return source.str();
  }};

  auto const &assetsPath{abcg::Application::getAssetsPath()};

  std::vector<abcg::ShaderSource> const sources{
      {.source =
           readFile(assetsPath / std::filesystem::path{kVertexShaderPath}),
       .stage = abcg::ShaderStage::Vertex},
      {.source =
           readFile(assetsPath / std::filesystem::path{kFragmentShaderPath}),
       .stage = abcg::ShaderStage::Fragment}};

  m_program = abcg::createOpenGLProgram(sources);

  // Get uniform locations of axes program
  m_modelMatrixLocation = abcg::glGetUniformLocation(m_program, "uModelMatrix");
  m_viewMatrixLocation = abcg::glGetUniformLocation(m_program, "uViewMatrix");
  m_projMatrixLocation = abcg::glGetUniformLocation(m_program, "uProjMatrix");
  m_normalMatrixLocation =
      abcg::glGetUniformLocation(m_program, "uNormalMatrix");
  m_instanceModelMatrixLocation =
      abcg::glGetUniformLocation(m_program, "uInstanceModelMatrix");
  m_instanceColorLocation =
      abcg::glGetUniformLocation(m_program, "uInstanceColor");
  m_radiusScaleLocation = abcg::glGetUniformLocation(m_program, "uRadiusScale");
  m_lengthScaleLocation = abcg::glGetUniformLocation(m_program, "uLengthScale");

  m_tickHalfWidthLocation =
      abcg::glGetUniformLocation(m_program, "uTickHalfWidth");
  m_cylinderHalfLengthLocation =
      abcg::glGetUniformLocation(m_program, "uCylinderHalfLength");
  m_cylinderRadiusLocation =
      abcg::glGetUniformLocation(m_program, "uCylinderRadius");
  m_lightDirectionLocation =
      abcg::glGetUniformLocation(m_program, "uLightDirection");

  // Create geometry for the canonical axis (unit cylinder+cone_tip aligned
  // along +X)
  std::vector<Vertex> vertices;
  std::vector<GLuint> indices;

  auto const halfLength{kBaseCylinderLength * 0.5f};

  auto const cylinderStart{-halfLength};
  auto const cylinderEnd{halfLength};
  geometry::createCylinder(vertices, indices, {cylinderStart, 0, 0},
                           {cylinderEnd, 0, 0}, kBaseCylinderRadius);

  auto const coneStart{cylinderEnd};
  auto const coneEnd{cylinderEnd + kConeLength};
  auto const coneRadius{kBaseCylinderRadius * 2.0f};
  geometry::createCone(vertices, indices, {coneStart, 0, 0}, {coneEnd, 0, 0},
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
  // NOLINTBEGIN(*reinterpret-cast, performance-no-int-to-ptr)
  abcg::glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
      reinterpret_cast<void *>(offsetof(Vertex, position)));
  // NOLINTEND(*reinterpret-cast, performance-no-int-to-ptr)

  // Normal attribute
  abcg::glEnableVertexAttribArray(1);
  // NOLINTBEGIN(*reinterpret-cast, performance-no-int-to-ptr)
  abcg::glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
      reinterpret_cast<void *>(offsetof(Vertex, normal)));
  // NOLINTEND(*reinterpret-cast, performance-no-int-to-ptr)

  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  abcg::glBindVertexArray(0);

  // Store axis tip positions for billboards
  m_xAxisTip = glm::vec3(coneEnd, 0, 0) + glm::vec3(0.075f, 0, 0);
  m_yAxisTip = glm::vec3(0, coneEnd, 0) + glm::vec3(0, 0.075f, 0);
  m_zAxisTip = glm::vec3(0, 0, coneEnd) + glm::vec3(0, 0, 0.075f);

  // Texture generated with https://evanw.github.io/font-texture-generator/
  auto const path{assetsPath / std::filesystem::path{kGlyphsTexturePath}};
  m_glyphsTexture = abcg::loadOpenGLTexture(
      {.path = path, .generateMipmaps = false, .flipUpsideDown = false});

  // Glyph | x_pos | y_pos | width | height | origin_x | origin_y
  // 'x'   |    71 |     0 |    71 |     69 |        4 |       63
  // 'y'   |     0 |     0 |    71 |     97 |        4 |       63
  // 'z'   |   142 |     0 |    62 |     69 |        3 |       63

  auto const texWidth{204.0f};
  auto const texHeight{97.0f};

  // X glyph
  m_glyphData[0].uv0 = glm::vec2(71.0f / texWidth, 1.0f);
  m_glyphData[0].uv1 = glm::vec2(142.0f / texWidth, 0.0f);
  m_glyphData[0].aspectRatio = 71.0f / texHeight;

  // Y glyph
  m_glyphData[1].uv0 = glm::vec2(0.0f / texWidth, 1.0f);
  m_glyphData[1].uv1 = glm::vec2(71.0f / texWidth, 0.0f);
  m_glyphData[1].aspectRatio = 71.0f / texHeight;

  // Z glyph
  m_glyphData[2].uv0 = glm::vec2(142.0f / texWidth, 1.0f);
  m_glyphData[2].uv1 = glm::vec2(1.0f, 0.0f);
  m_glyphData[2].aspectRatio = 62.0f / texHeight;

  createBillboards();

  // Create billboard shader program
  std::vector<abcg::ShaderSource> const billboardSources{
      {.source =
           readFile(assetsPath / std::filesystem::path{kGlyphVertexShaderPath}),
       .stage = abcg::ShaderStage::Vertex},
      {.source = readFile(assetsPath /
                          std::filesystem::path{kGlyphFragmentShaderPath}),
       .stage = abcg::ShaderStage::Fragment}};

  m_glyphProgram = abcg::createOpenGLProgram(billboardSources);

  // Get uniform locations of billboard program
  m_glyphViewMatrixLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uViewMatrix");
  m_glyphProjMatrixLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uProjMatrix");
  m_glyphModelMatrixLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uModelMatrix");
  m_glyphFontTextureLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uFontTexture");
  m_glyphTextColorLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uTextColor");
  m_glyphBillboardScaleLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uBillboardScale");
  m_glyphFadeAlphaLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uFadeAlpha");
  m_glyphBoundsRadiusLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uBoundsRadius");
  m_glyphCameraDistanceToOriginLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uCameraDistanceToOrigin");
  m_glyphBillboardPositionLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uBillboardPosition");
  m_glyphUV0Location = abcg::glGetUniformLocation(m_glyphProgram, "uGlyphUV0");
  m_glyphUV1Location = abcg::glGetUniformLocation(m_glyphProgram, "uGlyphUV1");
  m_glyphAspectRatioLocation =
      abcg::glGetUniformLocation(m_glyphProgram, "uAspectRatio");
}

void Axes::onDestroy() {
  abcg::glDeleteProgram(m_program);
  abcg::glDeleteVertexArrays(1, &m_VAO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteBuffers(1, &m_EBO);

  abcg::glDeleteProgram(m_glyphProgram);
  abcg::glDeleteVertexArrays(1, &m_glyphVAO);
  abcg::glDeleteBuffers(1, &m_glyphVBO);
  abcg::glDeleteTextures(1, &m_glyphsTexture);
}

void Axes::renderAxes(Camera const &camera) {
  if (!m_visible) {
    return;
  }

  auto const desiredWorldRadius{
      geometry::computeScreenSpaceRadius(camera, kTargetScreenRadius)};

  // Scale factor relative to the base cylinder radius
  auto const radiusScale{desiredWorldRadius / kBaseCylinderRadius};

  // Calculate length scale based on desired vs base length
  auto const lengthScale{m_desiredCylinderLength / kBaseCylinderLength};

  // Store for use in renderGlyphs
  m_lastRadiusScale = radiusScale;
  m_lastLengthScale = lengthScale;

  // Build instance transforms (assuming unit cylinder aligned along +X)
  std::array<glm::mat4, 3> instanceModels{};
  instanceModels[0] = glm::mat4(1.0f); // X axis: identity
  // Y axis: rotate +90 deg around Z to map +X -> +Y
  instanceModels[1] = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(),
                                  glm::vec3(0.0f, 0.0f, 1.0f));
  // Z axis: rotate -90 deg around Y to map +X -> +Z
  instanceModels[2] = glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(),
                                  glm::vec3(0.0f, 1.0f, 0.0f));

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
  abcg::glUniformMatrix4fv(m_instanceModelMatrixLocation, 3, GL_FALSE,
                           &instanceModels[0][0][0]);
  abcg::glUniform3fv(m_instanceColorLocation, 3, &kInstanceColors[0].x);
  abcg::glUniform1f(m_radiusScaleLocation, radiusScale);
  abcg::glUniform1f(m_lengthScaleLocation, lengthScale);

  auto const lightDirectionView{glm::normalize(
      glm::vec3(viewMatrix * modelMatrix * glm::vec4(m_lightDirection, 0.0)))};
  abcg::glUniform3fv(m_lightDirectionLocation, 1, &lightDirectionView[0]);
  abcg::glUniform1f(m_tickHalfWidthLocation, kHalfTickWidth);
  abcg::glUniform1f(m_cylinderHalfLengthLocation, kBaseCylinderLength * 0.5f);
  abcg::glUniform1f(m_cylinderRadiusLocation, kBaseCylinderRadius);

  abcg::glBindVertexArray(m_VAO);
  abcg::glDrawElementsInstanced(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT,
                                nullptr, 3);
  abcg::glBindVertexArray(0);

  abcg::glUseProgram(0);
}

void Axes::renderGlyphs(Camera const &camera, float boundsRadius,
                        bool fadeAlpha) {
  if (!m_visible) {
    return;
  }

  auto const modelMatrix{camera.getModelMatrix()};
  auto const viewMatrix{camera.getViewMatrix()};
  auto const projMatrix{camera.getProjMatrix()};

  // Render billboards
  abcg::glEnable(GL_BLEND);
  abcg::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  abcg::glUseProgram(m_glyphProgram);

  // Set matrices
  abcg::glUniformMatrix4fv(m_glyphViewMatrixLocation, 1, GL_FALSE,
                           &viewMatrix[0][0]);
  abcg::glUniformMatrix4fv(m_glyphProjMatrixLocation, 1, GL_FALSE,
                           &projMatrix[0][0]);
  abcg::glUniformMatrix4fv(m_glyphModelMatrixLocation, 1, GL_FALSE,
                           &modelMatrix[0][0]);

  auto const billboardScale{(0.3f * camera.getFovY() / 30.0f) /
                            (4.0f * camera.getModelScale())};
  abcg::glUniform1f(m_glyphBillboardScaleLocation, billboardScale);
  abcg::glUniform1i(m_glyphFadeAlphaLocation, static_cast<GLint>(fadeAlpha));
  abcg::glUniform1f(m_glyphBoundsRadiusLocation, boundsRadius);
  abcg::glUniform1f(m_glyphCameraDistanceToOriginLocation,
                    Camera::getLookAtDistance());

  abcg::glActiveTexture(GL_TEXTURE0);
  abcg::glBindTexture(GL_TEXTURE_2D, m_glyphsTexture);
  abcg::glUniform1i(m_glyphFontTextureLocation, 0);

  // Bind VAO
  abcg::glBindVertexArray(m_glyphVAO);
  abcg::glEnableVertexAttribArray(0);

  struct AxisLabel {
    glm::vec3 position;
    glm::vec3 color;
  };

  glm::vec3 const white{1.0f, 1.0f, 1.0f};

  // Scale the tip positions by radiusScale to match the scaled cones
  auto scaledXTip{m_xAxisTip};
  auto scaledYTip{m_yAxisTip};
  auto scaledZTip{m_zAxisTip};

  // The cone tips scale from their base, so we need to scale the offset from
  // the base
  auto const halfLength{kBaseCylinderLength * 0.5f};
  auto const cylinderEnd{halfLength};

  // Scale cylinder end by length scale
  auto const scaledCylinderEnd{cylinderEnd * m_lastLengthScale};

  // For each tip: scale the cylinder part by lengthScale,
  // and the cone offset by radiusScale only
  scaledXTip.x =
      scaledCylinderEnd + ((m_xAxisTip.x - cylinderEnd) * m_lastRadiusScale);
  scaledYTip.y =
      scaledCylinderEnd + ((m_yAxisTip.y - cylinderEnd) * m_lastRadiusScale);
  scaledZTip.z =
      scaledCylinderEnd + ((m_zAxisTip.z - cylinderEnd) * m_lastRadiusScale);

  std::array<AxisLabel, 3> const labels{
      {{.position = scaledXTip, .color = white},
       {.position = scaledYTip, .color = white},
       {.position = scaledZTip, .color = white}}};

  for (auto const index : iter::range(labels.size())) {
    abcg::glUniform3fv(m_glyphBillboardPositionLocation, 1,
                       &labels.at(index).position[0]);
    abcg::glUniform3fv(m_glyphTextColorLocation, 1, &labels.at(index).color[0]);
    abcg::glUniform2fv(m_glyphUV0Location, 1, &m_glyphData.at(index).uv0[0]);
    abcg::glUniform2fv(m_glyphUV1Location, 1, &m_glyphData.at(index).uv1[0]);
    abcg::glUniform1f(m_glyphAspectRatioLocation,
                      m_glyphData.at(index).aspectRatio);

    abcg::glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }

  abcg::glBindVertexArray(0);
  abcg::glUseProgram(0);
  abcg::glDisable(GL_BLEND);
}

void Axes::createBillboards() {
  std::vector<glm::vec2> const texCoords{
      {0, 0}, {1, 0}, {1, 1}, {0, 1}}; // BL, BR, TR, TL

  // Create VAO/VBO
  abcg::glGenVertexArrays(1, &m_glyphVAO);
  abcg::glGenBuffers(1, &m_glyphVBO);

  abcg::glBindVertexArray(m_glyphVAO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_glyphVBO);
  abcg::glBufferData(
      GL_ARRAY_BUFFER,
      gsl::narrow<GLsizeiptr>(texCoords.size() * sizeof(glm::vec2)),
      texCoords.data(), GL_STATIC_DRAW);

  // Vertex attribute: texCoord
  abcg::glEnableVertexAttribArray(0);
  abcg::glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2),
                              nullptr);

  abcg::glBindVertexArray(0);
}