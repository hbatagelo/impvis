/**
 * @file raycast.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef RAYCAST_HPP_
#define RAYCAST_HPP_

#include "camera.hpp"
#include "renderstate.hpp"

#include <abcgOpenGLShader.hpp>

class Raycast {
public:
  void handleEvent(SDL_Event const &event);
  void onCreate(RenderState const &renderState);
  void onUpdate();
  void onPaint(Camera const &camera, RenderState const &renderState,
               glm::quat lightRotation, std::function<void()> onFrameStart,
               std::function<void()> onFrameEnd);
  void onResize(glm::ivec2 size);
  void onDestroy();

  [[nodiscard]] bool isProgramValid() const noexcept { return !m_programBuildFailed; }

  [[nodiscard]] bool isFrameComplete() const noexcept {
    return !m_frameState.isRendering && m_frameState.frameCount > 0;
  }

  [[nodiscard]] std::size_t getFrameCount() const noexcept {
    return m_frameState.frameCount;
  }

  [[nodiscard]] float getRenderProgress() const noexcept {
    return std::clamp(gsl::narrow<float>(m_frameState.nextChunkY) /
                          gsl::narrow<float>(m_frameState.viewportSize.y),
                      0.0f, 1.0f);
  }

  [[nodiscard]] double getLastFrameTime() const noexcept {
    return m_frameState.lastFrameTime;
  }

  [[nodiscard]] int getNumRenderChunks() const noexcept {
    return gsl::narrow_cast<int>(m_frameState.numChunksEstimate);
  }

  [[nodiscard]] glm::vec3 getLightDirection() const noexcept {
    return m_shadingUBOData.lightDirWorld;
  }

  void setCompositionSource(GLuint colorTexture, GLuint depthTexture) noexcept {
    m_colorTexture = colorTexture;
    m_depthTexture = depthTexture;
  }

private:
  static constexpr std::string_view kVertexShaderPath{"shaders/raycast.vert"};
  static constexpr std::string_view kFragmentShaderPath{"shaders/raycast.frag"};
  static inline glm::vec3 kLightDirection{
      glm::normalize(glm::vec3{1.0f, -1.0f, -1.0f})};

  struct Vertex {
    glm::vec2 position{};
  };

  struct CameraUBOData {
    glm::vec3 eye{};
    float _pad0;

    glm::vec2 pixelSize{};
    float _pad1[2];

    glm::mat4 viewMatrix{};
    glm::mat4 invViewMatrix{};
    glm::mat4 projMatrix{};
    glm::mat4 invProjMatrix{};
    glm::mat4 modelMatrix{};
    glm::mat4 invModelMatrix{};

    glm::vec4 normalMatrixCol0{};
    glm::vec4 normalMatrixCol1{};
    glm::vec4 normalMatrixCol2{};

    float maxModelScale{};
    float _pad3[3];
  };
  static_assert(sizeof(CameraUBOData) == 480);

  struct ShadingUBOData {
    glm::vec3 insideKdId{};
    float _pad0;
    glm::vec3 outsideKdId{};
    float _pad1;

    glm::vec3 lightDirWorld{1.0f};
    float shininess{100.0f};
  };
  static_assert(sizeof(ShadingUBOData) == 48);

  struct ParamsUBOData {
    std::array<glm::vec4, 4> data;
  };

  // Minimum number of screen rows per chunk
  static constexpr int kMinChunkHeight{4};
  // Maximum time allowed for rendering a full frame. If the last frame render
  // took longer, the number of render chunks increases for the next frame.
  static constexpr double kFullFrameMaxTime{25.0 / 1000.0};

  struct FrameState {
    bool isRendering{};

    double numChunksEstimate{1.0};
    int chunkHeight{};
    int nextChunkY{};

    abcg::Timer frameTimer;

    RenderState capturedState;
    glm::ivec2 viewportSize{};

    std::size_t frameCount{};
    double lastFrameTime{};
  };
  FrameState m_frameState;

  CameraUBOData m_cameraUBOData;
  ShadingUBOData m_shadingUBOData;
  ParamsUBOData m_paramsUBOData;

  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_program{};
  GLuint m_UBOCamera{};
  GLuint m_UBOShading{};
  GLuint m_UBOParams{};
  GLint m_isoValueLocation{};
  GLint m_dvrAbsorptionCoeffLocation{};
  GLint m_dvrFalloffLocation{};
  GLint m_gaussianCurvatureFalloffLocation{};
  GLint m_meanCurvatureFalloffLocation{};
  GLint m_maxAbsCurvatureFalloffLocation{};
  GLint m_normalLengthFalloffLocation{};
  GLint m_colorTextureLocation{};
  GLint m_depthTextureLocation{};

  GLuint m_colorTexture{};
  GLuint m_depthTexture{};

  enum class ProgramBuildPhase { Compile, Link, Done };
  ProgramBuildPhase m_programBuildPhase{ProgramBuildPhase::Done};
  abcg::Timer m_programBuildTime;
  std::vector<abcg::OpenGLShader> m_shaderIDs;
  GLuint m_nextProgram{};
  bool m_throwOnProgramBuild{};
  bool m_programBuildFailed{};
#if defined(__EMSCRIPTEN__)
  bool m_documentVisible{};
#endif

  void createProgram(RenderState const &renderState);
  void createUBOs();
  void destroyUBOs();
  void createVBOs();
  void setupVAO();
  std::string getColormapDefinition(std::string_view name,
                                    std::vector<glm::vec4> const &colors);

  // Adaptive rendering
  void resetFrameState();
  void startNewFrame(RenderState const &renderState);
  void renderChunk(RenderState const &renderState);
  void onFrameCompleted();
  bool hasStateInvalidatedFrame(RenderState const &renderState) const noexcept;
};

#endif
