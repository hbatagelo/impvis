/**
 * @file renderstate.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef RENDERSTATE_HPP_
#define RENDERSTATE_HPP_

#include "function.hpp"

#include <glm/glm.hpp>

struct RenderState {
  enum class BoundsShape { Sphere, Box };
  enum class RenderingMode { LitSurface, UnlitSurface, DirectVolume };
  enum class SurfaceColorMode {
    SideSign,
    UnitNormal,
    NormalMagnitude,
    GaussianCurvature,
    MeanCurvature,
    MaxAbsCurvature,
  };
  enum class RootTestMode { SignChange, Taylor1stOrder, Taylor2ndOrder };
  enum class GradientMode {
    ForwardDifference,
    CentralDifference,
    FivePointStencil
  };

  Function function;

  float isoValue{0.0f};
  float dvrAbsorptionCoeff{5.0f};
  float dvrFalloff{1.0f};
  float gaussianCurvatureFalloff{1.0f};
  float meanCurvatureFalloff{1.0f};
  float maxAbsCurvatureFalloff{1.0f};
  float normalLengthFalloff{1.0f};

  glm::vec3 insideKdId{0.1f, 0.27f, 1.0f};  // #1b46ff
  glm::vec3 outsideKdId{1.0f, 0.27f, 0.1f}; // #ff461b

  BoundsShape boundsShape{BoundsShape::Sphere};
  float boundsRadius{2.5f};

  bool raymarchAdaptive{true};
  int isosurfaceRaymarchSteps{150};
  int dvrRaymarchSteps{450};
  RootTestMode raymarchRootTest{RootTestMode::SignChange};
  GradientMode raymarchGradientEvaluation{GradientMode::ForwardDifference};
  RenderingMode renderingMode{RenderingMode::LitSurface};
  SurfaceColorMode surfaceColorMode{SurfaceColorMode::SideSign};
  bool useShadows{true};
  bool useFog{true};
  bool showAxes{true};
  bool inwardNormals{true};

  int msaaSamples{1};

  std::vector<glm::vec4> maxAbsCurvColormap{
      {0.0f, 0.0f, 0.0f, 1.0f}, // #000000
      {1.0f, 0.0f, 0.0f, 1.0f}, // #ff0000
      {1.0f, 1.0f, 0.0f, 1.0f}, // #ffff00
      {1.0f, 1.0f, 1.0f, 1.0f}, // #ffffff
      {0.0f, 0.34f, 1.0f, 1.0f} // #0057ff
  };
  std::vector<glm::vec4> normalLengthColormap{
      {0.0f, 0.0f, 0.0f, 1.0f}, // #000000
      {1.0f, 0.0f, 0.0f, 1.0f}, // #ff0000
      {1.0f, 1.0f, 0.0f, 1.0f}, // #ffff00
      {1.0f, 1.0f, 1.0f, 1.0f}, // #ffffff
      {0.0f, 0.34f, 1.0f, 1.0f} // #0057ff
  };
  std::vector<glm::vec4> dvrColormap{
      {0.14f, 0.0f, 0.35f, 0.0f},  // #240065
      {0.14f, 0.0f, 0.35f, 0.75f}, // #240065
      {0.1f, 0.25f, 0.9f, 1.0f},   // #1a40e6
      {0.0f, 0.65f, 1.0f, 1.0f},   // #00a5ff
      {1.0f, 1.0f, 1.0f, 1.0f},    // #ffffff
      {1.0f, 0.76f, 0.0f, 1.0f},   // #ffc200
      {0.9f, 0.25f, 0.1f, 1.0f},   // #e6401a
      {0.5f, 0.0f, 0.0f, 0.75f},   // #7f0000
      {0.5f, 0.0f, 0.0f, 0.0f}     // #7f0000
  };
  std::vector<glm::vec4> curvatureColormap{
      {0.14f, 0.0f, 0.35f, 1.0f}, // #240065
      {0.1f, 0.25f, 0.9f, 1.0f},  // #1a40e6
      {0.0f, 0.65f, 1.0f, 1.0f},  // #00a5ff
      {1.0f, 1.0f, 1.0f, 1.0f},   // #ffffff
      {1.0f, 0.76f, 0.0f, 1.0f},  // #ffc200
      {0.9f, 0.25f, 0.1f, 1.0f},  // #e6401a
      {0.5f, 0.0f, 0.0f, 1.0f}    // #7f0000
  };

  friend bool operator==(RenderState const &,
                         RenderState const &) = default;
};

#endif
