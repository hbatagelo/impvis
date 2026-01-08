/**
 * @file axes.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef AXES_HPP_
#define AXES_HPP_

#include "camera.hpp"
#include "geometry.hpp"

class Axes {
public:
  void onCreate();
  void onDestroy();

  void renderAxes(Camera const &camera);
  void renderGlyphs(Camera const &camera, float boundsRadius, bool fadeAlpha);

  void setCylinderLength(float length) noexcept {
    m_desiredCylinderLength = length;
  }
  void setLightDirection(glm::vec3 direction) noexcept {
    m_lightDirection = direction;
  }

private:
  // Desired world-space radius (0.3% of screen height)
  static constexpr auto kTargetScreenRadius{0.003f};

  static constexpr auto kBaseCylinderLength{1.0f};
  static constexpr auto kBaseCylinderRadius{0.025f};
  static constexpr auto kConeLength{0.15f};
  static constexpr auto kHalfTickWidth{0.01f};

  static constexpr std::string_view kVertexShaderPath{"shaders/axes.vert"};
  static constexpr std::string_view kFragmentShaderPath{"shaders/axes.frag"};
  static constexpr std::string_view kGlyphVertexShaderPath{
      "shaders/glyph.vert"};
  static constexpr std::string_view kGlyphFragmentShaderPath{
      "shaders/glyph.frag"};
  static constexpr std::string_view kGlyphsTexturePath{
      "textures/glyphs_sdf.png"};

  using Vertex = geometry::Vertex;

  GLuint m_program{};
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_EBO{};

  GLint m_modelMatrixLocation{};
  GLint m_viewMatrixLocation{};
  GLint m_projMatrixLocation{};
  GLint m_normalMatrixLocation{};
  GLint m_instanceModelMatrixLocation{};
  GLint m_instanceColorLocation{};
  GLint m_radiusScaleLocation{};
  GLint m_lengthScaleLocation{};
  GLint m_cylinderHalfLengthLocation{};
  GLint m_lightDirectionLocation{};
  GLint m_tickHalfWidthLocation{};
  GLint m_cylinderRadiusLocation{};

  int m_numIndices{};
  bool m_visible{true};
  glm::vec3 m_lightDirection{0.0f, -1.0f, 0.0f};

  GLuint m_glyphsTexture{};
  GLuint m_glyphProgram{};
  GLuint m_glyphVAO{};
  GLuint m_glyphVBO{};

  GLint m_glyphModelMatrixLocation{};
  GLint m_glyphViewMatrixLocation{};
  GLint m_glyphProjMatrixLocation{};
  GLint m_glyphAspectRatioLocation{};
  GLint m_glyphBillboardScaleLocation{};
  GLint m_glyphBillboardPositionLocation{};
  GLint m_glyphFadeAlphaLocation{};
  GLint m_glyphBoundsRadiusLocation{};
  GLint m_glyphCameraDistanceToOriginLocation{};
  GLint m_glyphUV0Location{};
  GLint m_glyphUV1Location{};
  GLint m_glyphFontTextureLocation{};
  GLint m_glyphTextColorLocation{};

  struct GlyphData {
    glm::vec2 uv0;
    glm::vec2 uv1;
    float aspectRatio;
  };
  std::array<GlyphData, 3> m_glyphData{}; // For x, y, z

  glm::vec3 m_xAxisTip{};
  glm::vec3 m_yAxisTip{};
  glm::vec3 m_zAxisTip{};

  float m_desiredCylinderLength{kBaseCylinderLength};
  float m_lastRadiusScale{1.0f};
  float m_lastLengthScale{1.0f};

  void createBillboards();
};

#endif