/**
 * @file raycast.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#include <fstream>
#include <span>
#include <sstream>

#include "raycast.hpp"
#include "settings.hpp"
#include "util.hpp"

void RayCast::onEvent(SDL_Event const &event) {
  glm::ivec2 mousePosition;
  SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

  if (event.type == SDL_MOUSEMOTION) {
    m_trackBallCamera.mouseMove(mousePosition);
    m_trackBallLight.mouseMove(mousePosition);
  }
  if (event.type == SDL_MOUSEBUTTONDOWN) {
    if (event.button.button == SDL_BUTTON_LEFT) {
      m_trackBallCamera.mousePress(mousePosition);
    }
    if (event.button.button == SDL_BUTTON_RIGHT) {
      m_trackBallLight.mousePress(mousePosition);
    }
  }
  if (event.type == SDL_MOUSEBUTTONUP) {
    if (event.button.button == SDL_BUTTON_LEFT) {
      m_trackBallCamera.mouseRelease(mousePosition);
    }
    if (event.button.button == SDL_BUTTON_RIGHT) {
      m_trackBallLight.mouseRelease(mousePosition);
    }
  }
  if (event.type == SDL_MOUSEWHEEL) {
    m_lookAtDistance -= gsl::narrow<float>(event.wheel.y);

    auto const minDistance{0.1f};
    auto const maxDistance{100.0f};
    m_lookAtDistance = glm::clamp(m_lookAtDistance, minDistance, maxDistance);
  }
}

void RayCast::onCreate(Settings const &settings) {
  abcg::glGenFramebuffers(1, &m_FBO);
  createProgram(settings);
  createVBOs();

  // Initial trackball spin
  m_trackBallCamera.setAxis(glm::normalize(glm::vec3(1, 1, 1)));
  m_trackBallCamera.setVelocity(0.0001f);

  m_lastLookAtDistance = m_lookAtDistance;
}

void RayCast::onPaint(Settings const &settings, GLuint renderTexture) {
  if (settings.rebuildProgram) {
    // Disable exception throwing when creating user-defined shader programs to
    // avoid printing the information log to the console.
    // In the WebAssembly build, it also prevents throwing an uncaught JS
    // exception.
    m_throwOnBuild = settings.equation.getLoadedData().name != "User-defined";

    createProgram(settings);
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
    if (abcg::checkOpenGLShaderCompile(m_shaderIDs, m_throwOnBuild)) {
      m_nextProgram =
          abcg::triggerOpenGLShaderLink(m_shaderIDs, m_throwOnBuild);
      if (m_nextProgram != 0) {
        m_programBuildTime.restart();
        m_programBuildPhase = ProgramBuildPhase::Link;
      }
    } else {
      m_buildFailed = true;
    }
  }

  if (m_programBuildPhase == ProgramBuildPhase::Link &&
      m_programBuildTime.elapsed() >= buildPhaseTimeout) {
    if (abcg::checkOpenGLShaderLink(m_nextProgram, m_throwOnBuild)) {
      // Replace with new program
      abcg::glDeleteProgram(m_program);
      m_program = m_nextProgram;
      m_nextProgram = 0;
      m_buildFailed = false;
      createUBOs();
      setupVAO();
    } else {
      m_buildFailed = true;
    }
    m_programBuildPhase = ProgramBuildPhase::Done;
#if defined(__EMSCRIPTEN__)
    firstProgramBuild = false;
#endif
  }

  auto const lookAtDistance{m_programBuildPhase != ProgramBuildPhase::Done
                                ? m_lastLookAtDistance
                                : m_lookAtDistance};

  auto const camRot{glm::inverse(m_trackBallCamera.getRotation())};
  auto const camPos{camRot * glm::vec3{0.0f, 0.0f, lookAtDistance}};
  auto const camAt{glm::vec3{0.0f}};
  auto const camUp{camRot * glm::vec3{0.0f, 1.0f, 0.0f}};
  static auto const lightVector{glm::normalize(glm::vec3{-1.0f, 1.0f, 1.0f})};

  // Update data to be uploaded to UBO
  auto const viewMatrix{glm::lookAt(camPos, camAt, camUp)};
  m_transform.MV_I = glm::inverse(viewMatrix);
  m_camera.eye = camPos;
  m_shading.lightVector =
      glm::mat3(m_transform.MV_I) *
      glm::vec3{m_trackBallLight.getRotation() * lightVector};

  // Defer updating some UBO data to the end of the program build. This is to
  // avoid applying the recommended settings of the next program to the current
  // program.
  if (m_programBuildPhase == ProgramBuildPhase::Done) {
    m_camera.lookAtDistance = lookAtDistance;
    m_shading.gaussianEps = settings.colormapScale;

    for (auto &&[index, param] :
         iter::enumerate(settings.equation.getParameters())) {
      auto const vecIndex{index / 4};
      auto const varIndex{gsl::narrow<int>(index % 4)};
      m_params.data.at(vecIndex)[varIndex] = param.value;
    }

    m_lastLookAtDistance = lookAtDistance;
  }

  if (renderTexture > 0) {
    abcg::glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    abcg::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, renderTexture, 0);
  }

  abcg::glDisable(GL_DEPTH_TEST);

  if (m_program != 0) {
    abcg::glUseProgram(m_program);

    auto updateUBO{[]<typename T>(GLuint buffer, std::span<T> const span) {
      abcg::glBindBuffer(GL_UNIFORM_BUFFER, buffer);
      abcg::glBufferSubData(GL_UNIFORM_BUFFER, 0,
                            gsl::narrow<GLsizeiptr>(span.size()), span.data());
      abcg::glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }};
    updateUBO(m_UBOCamera, std::span{&m_camera, sizeof(m_camera)});
    updateUBO(m_UBOShading, std::span{&m_shading, sizeof(m_shading)});
    updateUBO(m_UBOTransform, std::span{&m_transform, sizeof(m_transform)});
    updateUBO(m_UBOParams, std::span{&m_params, sizeof(m_params)});

    abcg::glUniform1f(m_isoValueLocation, settings.isoValue);

    abcg::glBindVertexArray(m_VAO);
    abcg::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    abcg::glBindVertexArray(0);

    abcg::glUseProgram(0);
  }

  if (renderTexture > 0) {
    abcg::glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

void RayCast::onResize(glm::ivec2 const &size) {
  auto const aspectRatio{gsl::narrow<float>(size.x) /
                         gsl::narrow<float>(size.y)};
  m_camera.scale = aspectRatio > 1.0f ? glm::vec2(aspectRatio, 1.0f)
                                      : glm::vec2(1.0f, 1.0f / aspectRatio);

  m_trackBallCamera.resizeViewport(size);
  m_trackBallLight.resizeViewport(size);
}

void RayCast::onDestroy() {
  abcg::glDeleteVertexArrays(1, &m_VAO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteBuffers(1, &m_UBOParams);
  abcg::glDeleteBuffers(1, &m_UBOTransform);
  abcg::glDeleteBuffers(1, &m_UBOShading);
  abcg::glDeleteBuffers(1, &m_UBOCamera);
  abcg::glDeleteProgram(m_program);
  abcg::glDeleteFramebuffers(1, &m_FBO);
}

void RayCast::createUBOs() {
  destroyUBOs();

  auto createUBO{[this]<typename T>(T const &data, GLuint bindingPoint,
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
      throw abcg::RuntimeError(fmt::format(
          "\"{}\" does not identify an active uniform block of program",
          uniformBlockName));
    }
    abcg::glUniformBlockBinding(m_program, index, bindingPoint);

    return buffer;
  }};

  // Create UBOs
  m_UBOCamera = createUBO(m_camera, 0, "CameraBlock");
  m_UBOShading = createUBO(m_shading, 1, "ShadingBlock");
  m_UBOTransform = createUBO(m_transform, 2, "TransformBlock");
  m_UBOParams = createUBO(m_params, 3, "ParamsBlock");

  // Get location of other uniform variables
  m_isoValueLocation = abcg::glGetUniformLocation(m_program, "uIsoValue");
}

void RayCast::destroyUBOs() {
  abcg::glDeleteBuffers(1, &m_UBOParams);
  abcg::glDeleteBuffers(1, &m_UBOTransform);
  abcg::glDeleteBuffers(1, &m_UBOShading);
  abcg::glDeleteBuffers(1, &m_UBOCamera);
}

void RayCast::createVBOs() {
  abcg::glDeleteBuffers(1, &m_VBO);

  abcg::glGenBuffers(1, &m_VBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  std::array const vertices{
      Vertex{.position = {-1, +1}}, Vertex{.position = {-1, -1}},
      Vertex{.position = {+1, +1}}, Vertex{.position = {+1, -1}}};
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(),
                     GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RayCast::createProgram(Settings const &settings) {
  // Reads a file and returns its contents as a string
  auto readFile{[](std::string_view filename) {
    std::stringstream shaderSource;
    if (std::ifstream stream(filename.data()); stream) {
      shaderSource << stream.rdbuf();
      stream.close();
    } else {
      throw abcg::RuntimeError(
          fmt::format("Failed to read shader file {}", filename));
    }
    return shaderSource.str();
  }};

  static auto const &assetsPath{abcg::Application::getAssetsPath()};
  std::vector<abcg::ShaderSource> sources{
      {.source = readFile(assetsPath + m_vertexShaderPath),
       .stage = abcg::ShaderStage::Vertex},
      {.source = readFile(assetsPath + m_fragmentShaderPath),
       .stage = abcg::ShaderStage::Fragment}};

  // Replace placeholders
  std::string definitions{};
  if (settings.renderSettings.useBoundingBox) {
    definitions += "#define USE_BOUNDING_BOX\n";
  }
  if (settings.renderSettings.rayMarchSignTest) {
    definitions += "#define USE_SIGN_TEST\n";
  }
  if (settings.renderSettings.rayMarchAdaptive) {
    definitions += "#define USE_ADAPTIVE_RAY_MARCH\n";
  }
  if (settings.renderSettings.shaderIndex == 0) {
    definitions += "#define USE_BLINN_PHONG\n";
  }
  if (settings.renderSettings.shaderIndex != 2) {
    definitions += "#define SHOW_ISOSURFACE\n";
  }
  if (settings.renderSettings.useShadows) {
    definitions += "#define USE_SHADOWS\n";
  }
  if (settings.renderSettings.useFog) {
    definitions += "#define USE_FOG\n";
  }
  if (settings.renderSettings.useNormalsAsColors) {
    definitions += "#define USE_NORMALS_AS_COLORS\n";
  }

  definitions += "#define RAY_MARCH_STEPS " +
                 std::to_string(settings.renderSettings.rayMarchSteps) + '\n';

  auto &fragmentShader{sources.at(1)};
  replaceAll(fragmentShader.source, "@DEFINITIONS@", definitions);

  replaceAll(fragmentShader.source, "@ISOVALUE@",
             std::to_string(settings.isoValue));

  replaceAll(fragmentShader.source, "@BOUND_RADIUS@",
             std::to_string(settings.renderSettings.boundRadius));

  auto const &loadedData{settings.equation.getLoadedData()};
  std::string const codeLocal{loadedData.codeLocal};
  std::string const codeGlobal{loadedData.codeGlobal};

  std::string equation{settings.equation.getGLSLExpression()};

  // Replace parameter names with uParams.data[index]
  for (auto &&[index, param] :
       iter::enumerate(settings.equation.getParameters())) {

    auto const vecIndex{index / 4};
    static std::array const variables{'x', 'y', 'z', 'w'};
    auto const var{variables.at(index % 4)};

    replaceAll(equation, param.name,
               fmt::format("uParams.data[{}].{}", vecIndex, var).c_str(), true);
  }

  // This replacement must be performed AFTER the replacement of parameter names
  // with uParams.data[index]. Otherwise, "p" can be wrongly interpreted as a
  // parameter.
  replaceAll(equation, "@P.@", "p.");

  replaceAll(fragmentShader.source, "@CODE_LOCAL@", codeLocal);
  replaceAll(fragmentShader.source, "@CODE_GLOBAL@", codeGlobal);
  replaceAll(fragmentShader.source, "@EQUATION@", equation);

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
}

void RayCast::setupVAO() {
  abcg::glDeleteVertexArrays(1, &m_VAO);

  abcg::glGenVertexArrays(1, &m_VAO);
  abcg::glBindVertexArray(m_VAO);

  auto const setUpVertexAttribute{[&](auto name, auto size, intptr_t offset) {
    auto const location{abcg::glGetAttribLocation(m_program, name)};
    if (location >= 0) {
      abcg::glEnableVertexAttribArray(gsl::narrow<GLuint>(location));

      abcg::glVertexAttribPointer(gsl::narrow<GLuint>(location), size, GL_FLOAT,
                                  GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void *>(offset)); // NOLINT
    } else {
      abcg::glDeleteVertexArrays(1, &m_VAO);
      throw abcg::RuntimeError(fmt::format("Failed to find attribute {} in {}",
                                           name, m_vertexShaderPath));
    }
  }};
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  setUpVertexAttribute("inPosition", 2, 0);

  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  abcg::glBindVertexArray(0);
}