/**
 * @file raycast.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "raycast.hpp"

#include "renderstate.hpp"
#include "util.hpp"

#include <abcgOpenGL.hpp>

#include <fstream>

namespace {

std::string getColormapDefinition(std::string_view name,
                                  std::vector<glm::vec4> const &colors) {
  auto const emitArray{[&name](std::vector<glm::vec4> const &lut) {
    auto out{std::format("const vec4 {}[{}] = vec4[{}](\n", name, lut.size(),
                         lut.size())};
    for (std::size_t const index : iter::range(lut.size())) {
      auto const &color{lut[index]};
      out += std::format("    vec4({:.6f}, {:.6f}, {:.6f}, {:.6f}){}", color.r,
                         color.g, color.b, color.a,
                         (index + 1 < lut.size() ? ",\n" : "\n"));
    }
    out += ");\n\n";
    return out;
  }};

  std::string str;
  str.reserve(32768);

  str += emitArray(colors);
  str += std::format("const int {}_SIZE = {};\n", name, colors.size());

  return str;
}

} // namespace

void Raycast::handleEvent(SDL_Event const &event) {
  if (event.type == SDL_EVENT_WINDOW_RESTORED ||
      event.type == SDL_EVENT_WINDOW_SHOWN ||
      event.type == SDL_EVENT_WINDOW_EXPOSED) {
    resetFrameState();
  }
}

void Raycast::onCreate(RenderState const &renderState) {
  createProgram(renderState);
  createVBOs();

#if defined(__EMSCRIPTEN__)
  m_documentVisible =
      gsl::narrow<bool>(EM_ASM_INT({ return !!Module.documentVisible; }));
#endif
}

void Raycast::onUpdate() {
#if defined(__EMSCRIPTEN__)
  auto const documentVisible{
      gsl::narrow<bool>(EM_ASM_INT({ return !!Module.documentVisible; }))};
  if (documentVisible != m_documentVisible) {
    m_documentVisible = documentVisible;
    resetFrameState();
  }
#endif
}

void Raycast::onPaint(Camera const &camera, RenderState const &renderState,
                      glm::quat lightRotation) {

  if (hasStateInvalidatedFrame(renderState)) {
    // Disable exception throwing when creating user-defined shader programs to
    // avoid printing the information log to the console.
    // In the WebAssembly build, it also prevents throwing an uncaught JS
    // exception.
    m_throwOnProgramBuild =
        renderState.function.getData().name != "User-defined";

    resetFrameState();
    createProgram(renderState);
  }

#if defined(__EMSCRIPTEN__)
  static bool firstProgramBuild{true};
  // Postpone compile/link status checking to avoid stalling the browser if it
  // takes too long to build the shader.
  auto const buildPhaseTimeout{firstProgramBuild ? 0.1 : 0.05};
#else
  auto const buildPhaseTimeout{0.0};
#endif

  if (m_programBuildPhase == ProgramBuildPhase::Compile &&
      m_programBuildTime.elapsed() >= buildPhaseTimeout) {
    m_programBuildPhase = ProgramBuildPhase::Done;
    if (abcg::checkOpenGLShaderCompile(m_shaderIDs, m_throwOnProgramBuild)) {
      m_nextProgram =
          abcg::triggerOpenGLShaderLink(m_shaderIDs, m_throwOnProgramBuild);
      if (m_nextProgram != 0) {
        m_programBuildTime.restart();
        m_programBuildPhase = ProgramBuildPhase::Link;
      }
    } else {
      m_programBuildFailed = true;
    }
  }

  if (m_programBuildPhase == ProgramBuildPhase::Link &&
      m_programBuildTime.elapsed() >= buildPhaseTimeout) {
    if (abcg::checkOpenGLShaderLink(m_nextProgram, m_throwOnProgramBuild)) {
      // Replace with new program
      abcg::glDeleteProgram(m_program);
      m_program = m_nextProgram;
      m_nextProgram = 0;
      m_programBuildFailed = false;
      createUBOs();
      setupVAO();
    } else {
      m_programBuildFailed = true;
    }
    m_programBuildPhase = ProgramBuildPhase::Done;
#if defined(__EMSCRIPTEN__)
    firstProgramBuild = false;
#endif
  }

  if (m_programBuildPhase != ProgramBuildPhase::Done) {
    return;
  }

  if (!m_frameState.isRendering || hasStateInvalidatedFrame(renderState)) {
    if (!m_frameState.isRendering) {
      onFrameCompleted();
    }
    startNewFrame(renderState);

    // Update camera and shading UBO data
    m_cameraUBOData.eye = camera.getPosition();
    m_cameraUBOData.pixelSize = camera.getPixelSize();
    m_cameraUBOData.viewMatrix = camera.getViewMatrix();
    m_cameraUBOData.invViewMatrix = camera.getInvViewMatrix();
    m_cameraUBOData.projMatrix = camera.getProjMatrix();
    m_cameraUBOData.invProjMatrix = camera.getInvProjMatrix();
    m_cameraUBOData.modelMatrix = camera.getModelMatrix();
    m_cameraUBOData.invModelMatrix = camera.getInvModelMatrix();

    auto const normalMatrix{camera.getNormalMatrix()};
    m_cameraUBOData.normalMatrixCol0 = glm::vec4(normalMatrix[0], 0.0f);
    m_cameraUBOData.normalMatrixCol1 = glm::vec4(normalMatrix[1], 0.0f);
    m_cameraUBOData.normalMatrixCol2 = glm::vec4(normalMatrix[2], 0.0f);

    m_cameraUBOData.maxModelScale = camera.getModelScale();

    m_shadingUBOData.insideKdId = renderState.insideKdId;
    m_shadingUBOData.outsideKdId = renderState.outsideKdId;
    m_shadingUBOData.lightDirWorld =
        glm::mat3(m_cameraUBOData.invViewMatrix) *
        glm::normalize(glm::vec3{lightRotation * kLightDirection});

    for (auto &&[index, param] :
         iter::enumerate(renderState.function.getParameters())) {
      auto const vecIndex{index / 4};
      auto const varIndex{gsl::narrow<int>(index % 4)};
      m_paramsUBOData.data.at(vecIndex)[varIndex] = param.value;
    }

    if (m_onFrameStart) {
      m_onFrameStart();
    }
  }

  if (m_frameState.isRendering) {
    renderChunk(renderState);
    if (m_frameState.nextChunkY >= m_frameState.viewportSize.y) {
      m_frameState.isRendering = false;

      if (m_onFrameEnd) {
        m_onFrameEnd();
      }
    }
  }
}

void Raycast::onDestroy() {
  abcg::glDeleteVertexArrays(1, &m_VAO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteBuffers(1, &m_UBOParams);
  abcg::glDeleteBuffers(1, &m_UBOShading);
  abcg::glDeleteBuffers(1, &m_UBOCamera);
  abcg::glDeleteProgram(m_program);
}

void Raycast::createProgram(RenderState const &renderState) {
  static auto const &assetsPath{abcg::Application::getAssetsPath()};

  auto const readFile{[](std::filesystem::path const &path) -> std::string {
    auto const utf8Path{abcg::pathToUtf8(path)};
    std::ifstream stream(utf8Path, std::ios::binary);
    if (!stream) {
      throw abcg::RuntimeError(std::format("Failed to read file {}", utf8Path));
    }

    std::ostringstream ss;
    ss << stream.rdbuf();
    return ss.str();
  }};

  std::vector<abcg::ShaderSource> sources{
      {.source =
           readFile(assetsPath / std::filesystem::path{kVertexShaderPath}),
       .stage = abcg::ShaderStage::Vertex},
      {.source =
           readFile(assetsPath / std::filesystem::path{kFragmentShaderPath}),
       .stage = abcg::ShaderStage::Fragment}};

  // Replace placeholders
  std::string definitions{};
  if (renderState.boundsShape == RenderState::BoundsShape::Box) {
    definitions += "#define USE_BOUNDING_BOX\n";
  }

  if (renderState.raymarchRootTest == RenderState::RootTestMode::SignChange) {
    definitions += "#define USE_SIGN_TEST\n";
  } else if (renderState.raymarchRootTest ==
             RenderState::RootTestMode::Taylor1stOrder) {
    definitions += "#define USE_TAYLOR_1ST\n";
  } else if (renderState.raymarchRootTest ==
             RenderState::RootTestMode::Taylor2ndOrder) {
    definitions += "#define USE_TAYLOR_2ND\n";
  }

  if (renderState.raymarchGradientEvaluation ==
      RenderState::GradientMode::CentralDifference) {
    definitions += "#define GRADIENT_CENTRAL_DIFFERENCE\n";
  } else if (renderState.raymarchGradientEvaluation ==
             RenderState::GradientMode::FivePointStencil) {
    definitions += "#define GRADIENT_FIVE_POINT_STENCIL\n";
  }

  if (renderState.raymarchAdaptive) {
    definitions += "#define USE_ADAPTIVE_RAY_MARCH\n";
  }

  if (renderState.renderingMode == RenderState::RenderingMode::LitSurface) {
    definitions += "#define USE_BLINN_PHONG\n";
  }

  if (renderState.renderingMode == RenderState::RenderingMode::LitSurface ||
      renderState.renderingMode == RenderState::RenderingMode::UnlitSurface) {
    definitions += "#define SHOW_ISOSURFACE\n";

    if (renderState.msaaSamples > 1) {
      definitions += "#define MSAA_ENABLED\n";
      definitions += std::format("#define MSAA_{}X\n", renderState.msaaSamples);
    }
  }

  if (renderState.useShadows) {
    definitions += "#define USE_SHADOWS\n";
  }

  if (renderState.useFog) {
    definitions += "#define USE_FOG\n";
  }

  if (renderState.surfaceColorMode ==
      RenderState::SurfaceColorMode::UnitNormal) {
    definitions += "#define SHOW_NORMAL_VECTOR\n";
  } else if (renderState.surfaceColorMode ==
             RenderState::SurfaceColorMode::GaussianCurvature) {
    definitions += "#define USE_CURVATURE\n";
    definitions += "#define SHOW_GAUSSIAN_CURVATURE\n";
  } else if (renderState.surfaceColorMode ==
             RenderState::SurfaceColorMode::MeanCurvature) {
    definitions += "#define USE_CURVATURE\n";
    definitions += "#define SHOW_MEAN_CURVATURE\n";
  } else if (renderState.surfaceColorMode ==
             RenderState::SurfaceColorMode::MaxAbsCurvature) {
    definitions += "#define USE_CURVATURE\n";
    definitions += "#define SHOW_MAX_CURVATURE\n";
  } else if (renderState.surfaceColorMode ==
             RenderState::SurfaceColorMode::NormalMagnitude) {
    definitions += "#define SHOW_NORMAL_MAGNITUDE\n";
  }

  if (renderState.inwardNormals) {
    definitions += "#define INWARD_NORMALS\n";
  }

  if (renderState.showAxes) {
    definitions += "#define SHOW_AXES\n";
  }

  definitions += "#define ISOSURFACE_RAYMARCH_STEPS " +
                 std::to_string(renderState.isosurfaceRaymarchSteps) + '\n';
  definitions += "#define DVR_RAYMARCH_STEPS " +
                 std::to_string(renderState.dvrRaymarchSteps) + '\n';

  definitions += getColormapDefinition(
      "SEQ_COLORMAP", renderState.surfaceColorMode ==
                              RenderState::SurfaceColorMode::NormalMagnitude
                          ? renderState.normalLengthColormap
                          : renderState.maxAbsCurvColormap);

  definitions += getColormapDefinition(
      "DIV_COLORMAP",
      renderState.renderingMode == RenderState::RenderingMode::DirectVolume
          ? renderState.dvrColormap
          : renderState.curvatureColormap);

  auto &fragmentShader{sources.at(1)};
  util::replaceAll(fragmentShader.source, "@DEFINITIONS@", definitions);

  util::replaceAll(fragmentShader.source, "@ISOVALUE@",
                     std::to_string(renderState.isoValue));

  util::replaceAll(fragmentShader.source, "@BOUND_RADIUS@",
                     std::to_string(renderState.boundsRadius));

  auto const &data{renderState.function.getData()};
  std::string const codeLocal{data.codeLocal};
  std::string const codeGlobal{data.codeGlobal};

  std::string expression{renderState.function.getGLSLExpression()};

  // Replace parameter names with uParams.data[index]
  for (auto &&[index, param] :
       iter::enumerate(renderState.function.getParameters())) {

    auto const vecIndex{index / 4};
    static std::array const variables{'x', 'y', 'z', 'w'};
    auto const var{variables.at(index % 4)};

    util::replaceAll(expression, param.name,
                       std::format("uParams.data[{}].{}", vecIndex, var), true);
  }

  // This replacement must be performed AFTER the replacement of parameter names
  // with uParams.data[index]. Otherwise, "P" can be wrongly interpreted as
  // a parameter.
  util::replaceAll(expression, "@P.@", "P.");

  util::replaceAll(fragmentShader.source, "@CODE_LOCAL@", codeLocal);
  util::replaceAll(fragmentShader.source, "@CODE_GLOBAL@", codeGlobal);
  util::replaceAll(fragmentShader.source, "@EXPRESSION_LHS@", expression);

  // Interrupted during building?
  if (m_programBuildPhase != ProgramBuildPhase::Done) {
    // Delete shaders
    for (auto const &shaderID : m_shaderIDs) {
      if (shaderID.shader == 0) {
        continue;
      }
      abcg::glDeleteShader(shaderID.shader);
    }
  }
  // Interrupted during linking?
  if (m_programBuildPhase == ProgramBuildPhase::Link) {
    abcg::glDeleteProgram(m_nextProgram);
  }

  m_programBuildTime.restart();
  m_shaderIDs = abcg::triggerOpenGLShaderCompile(sources);
  m_programBuildPhase = ProgramBuildPhase::Compile;

  m_frameState.capturedState = renderState;
}

void Raycast::createUBOs() {
  destroyUBOs();

  auto createUBO{[&]<typename T>(T const &data, GLuint bindingPoint,
                                 GLchar const *uniformBlockName) {
    GLuint buffer{};
    abcg::glGenBuffers(1, &buffer);

    // Set the data
    abcg::glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    abcg::glBufferData(GL_UNIFORM_BUFFER, sizeof(data), &data, GL_DYNAMIC_DRAW);
    abcg::glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Link the buffer to a binding point
    abcg::glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, buffer);

    // Connect the binding point to the block index of the program
    auto const index{abcg::glGetUniformBlockIndex(m_program, uniformBlockName)};
    if (index == GL_INVALID_INDEX) {
      destroyUBOs();
      throw abcg::RuntimeError(std::format(
          "\"{}\" does not identify an active uniform block of program",
          uniformBlockName));
    }
    abcg::glUniformBlockBinding(m_program, index, bindingPoint);

    return buffer;
  }};

  m_UBOCamera = createUBO(m_cameraUBOData, 0, "CameraBlock");
  m_UBOShading = createUBO(m_shadingUBOData, 1, "ShadingBlock");
  m_UBOParams = createUBO(m_paramsUBOData, 2, "ParamsBlock");

  // Get location of other uniform variables
  m_isoValueLocation = abcg::glGetUniformLocation(m_program, "uIsoValue");
  m_dvrDensityLocation = abcg::glGetUniformLocation(m_program, "uDVRDensity");
  m_dvrFalloffLocation = abcg::glGetUniformLocation(m_program, "uDVRFalloff");
  m_gaussianCurvatureFalloffLocation =
      abcg::glGetUniformLocation(m_program, "uGaussianCurvatureFalloff");
  m_meanCurvatureFalloffLocation =
      abcg::glGetUniformLocation(m_program, "uMeanCurvatureFalloff");
  m_maxAbsCurvatureFalloffLocation =
      abcg::glGetUniformLocation(m_program, "uMaxAbsCurvatureFalloff");
  m_normalLengthFalloffLocation =
      abcg::glGetUniformLocation(m_program, "uNormalLengthFalloff");
  m_colorTextureLocation =
      abcg::glGetUniformLocation(m_program, "uColorTexture");
  m_depthTextureLocation =
      abcg::glGetUniformLocation(m_program, "uDepthTexture");
}

void Raycast::destroyUBOs() {
  abcg::glDeleteBuffers(1, &m_UBOParams);
  abcg::glDeleteBuffers(1, &m_UBOShading);
  abcg::glDeleteBuffers(1, &m_UBOCamera);
}

void Raycast::createVBOs() {
  abcg::glDeleteBuffers(1, &m_VBO);

  abcg::glGenBuffers(1, &m_VBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  std::array const vertices{Vertex{{+3, -1}}, Vertex{{-1, +3}},
                            Vertex{{-1, -1}}};
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(),
                     GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Raycast::setupVAO() {
  abcg::glDeleteVertexArrays(1, &m_VAO);

  abcg::glGenVertexArrays(1, &m_VAO);
  abcg::glBindVertexArray(m_VAO);

  auto const setUpVertexAttribute{[&](auto name, auto size, intptr_t offset) {
    auto const location{abcg::glGetAttribLocation(m_program, name)};
    if (location >= 0) {
      abcg::glEnableVertexAttribArray(gsl::narrow<GLuint>(location));
      // NOLINTBEGIN(*reinterpret-cast, performance-no-int-to-ptr)
      abcg::glVertexAttribPointer(gsl::narrow<GLuint>(location), size, GL_FLOAT,
                                  GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void *>(offset));
      // NOLINTEND(*reinterpret-cast, performance-no-int-to-ptr)
    } else {
      abcg::glDeleteVertexArrays(1, &m_VAO);
      throw abcg::RuntimeError(std::format("Failed to find attribute {} in {}",
                                           name, kVertexShaderPath));
    }
  }};

  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  setUpVertexAttribute("inPosition", 2, 0);

  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  abcg::glBindVertexArray(0);
}

void Raycast::resetFrameState() {
  m_frameState.frameTimer.restart();
  m_frameState.numChunksEstimate = 1.0;
  m_frameState.isRendering = false;
  m_frameState.nextChunkY = 0;
  m_frameState.chunkHeight = 0;
  m_frameState.lastFrameTime = 0.0;
}

void Raycast::startNewFrame(RenderState const &renderState) {
  m_frameState.frameTimer.restart();
  m_frameState.isRendering = true;
  m_frameState.capturedState = renderState;
  m_frameState.nextChunkY = 0;
  m_frameState.chunkHeight =
      std::max(1, m_frameState.viewportSize.y /
                      gsl::narrow_cast<int>(m_frameState.numChunksEstimate));
}

void Raycast::renderChunk(RenderState const &renderState) {
  if (m_program == 0) {
    return;
  }

  auto const chunkY{m_frameState.nextChunkY};
  auto const chunkHeight{
      std::min(m_frameState.chunkHeight, m_frameState.viewportSize.y - chunkY)};
  if (chunkHeight <= 0) {
    return;
  }

  abcg::glEnable(GL_DEPTH_TEST);
  abcg::glDepthFunc(GL_ALWAYS);
  abcg::glDepthMask(GL_TRUE);

  abcg::glEnable(GL_SCISSOR_TEST);
  abcg::glScissor(0, chunkY, m_frameState.viewportSize.x, chunkHeight);

  abcg::glUseProgram(m_program);

  auto const updateUBO{[]<typename T>(GLuint buffer, std::span<T> const span) {
    abcg::glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    abcg::glBufferSubData(GL_UNIFORM_BUFFER, 0,
                          gsl::narrow<GLsizeiptr>(span.size()), span.data());
    abcg::glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }};
  updateUBO(m_UBOCamera, std::span{&m_cameraUBOData, sizeof(m_cameraUBOData)});
  updateUBO(m_UBOShading,
            std::span{&m_shadingUBOData, sizeof(m_shadingUBOData)});
  updateUBO(m_UBOParams, std::span{&m_paramsUBOData, sizeof(m_paramsUBOData)});

  abcg::glUniform1f(m_isoValueLocation, renderState.isoValue);
  abcg::glUniform1f(m_dvrDensityLocation, renderState.dvrDensity);
  abcg::glUniform1f(m_dvrFalloffLocation, renderState.dvrFalloff);
  abcg::glUniform1f(m_gaussianCurvatureFalloffLocation,
                    renderState.gaussianCurvatureFalloff);
  abcg::glUniform1f(m_meanCurvatureFalloffLocation,
                    renderState.meanCurvatureFalloff);
  abcg::glUniform1f(m_maxAbsCurvatureFalloffLocation,
                    renderState.maxAbsCurvatureFalloff);
  abcg::glUniform1f(m_normalLengthFalloffLocation,
                    renderState.normalLengthFalloff);

  if (renderState.showAxes) {
    if (m_depthTextureGetter) {
      if (auto const depthTexture{m_depthTextureGetter()}; depthTexture > 0) {
        abcg::glActiveTexture(GL_TEXTURE0);
        abcg::glBindTexture(GL_TEXTURE_2D, depthTexture);
        abcg::glUniform1i(m_depthTextureLocation, 0);
      }
    }

    if (m_colorTextureGetter) {
      if (auto const colorTexture{m_colorTextureGetter()}; colorTexture > 0) {
        abcg::glActiveTexture(GL_TEXTURE1);
        abcg::glBindTexture(GL_TEXTURE_2D, colorTexture);
        abcg::glUniform1i(m_colorTextureLocation, 1);
      }
    }
  }

  abcg::glBindVertexArray(m_VAO);
  abcg::glDrawArrays(GL_TRIANGLES, 0, 3);
  abcg::glBindVertexArray(0);

  abcg::glUseProgram(0);
  abcg::glDisable(GL_SCISSOR_TEST);
  abcg::glDepthFunc(GL_LESS);
  abcg::glDisable(GL_DEPTH_TEST);

  m_frameState.nextChunkY += chunkHeight;
}

void Raycast::onFrameCompleted() {
  auto const fps{gsl::narrow<double>(ImGui::GetIO().Framerate)};
  auto const deltaFPSNormalized{(kMinimumUIFPS - fps) / kMinimumUIFPS};
  auto const newNumChunksEstimate{m_frameState.numChunksEstimate +
                                  deltaFPSNormalized};

  m_frameState.numChunksEstimate = std::clamp(
      newNumChunksEstimate, 1.0, gsl::narrow<double>(kMaxTotalChunks));

  m_frameState.lastFrameTime = m_frameState.frameTimer.elapsed();

  ++m_frameState.frameCount;
}

void Raycast::onResize(glm::ivec2 size) { m_frameState.viewportSize = size; }

bool Raycast::hasStateInvalidatedFrame(
    RenderState const &renderState) const noexcept {
  auto const &captured{m_frameState.capturedState};
  auto const &current{renderState};

  return captured != current;
}