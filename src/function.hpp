/**
 * @file function.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef FUNCTION_HPP_
#define FUNCTION_HPP_

#include <abcgOpenGLExternal.hpp>

#include <string>
#include <vector>

class Function {
public:
  struct Parameter {
    std::string name;
    float value{};

    friend bool operator==(Parameter const &, Parameter const &) = default;
  };

  struct Data {
    std::string name{"Default"};
    std::string thumbnail;
    std::string expression;
    std::string codeLocal;
    std::string codeGlobal;
    std::string comment;
    std::string boundsShape{"sphere"};
    float boundsRadius{2.5f};
    std::string isosurfaceRaymarchMethod{"adaptive"};
    int isosurfaceRaymarchSteps{150};
    int dvrRaymarchSteps{150};
    std::string isosurfaceRaymarchRootTest{"sign change"};
    std::string isosurfaceRaymarchGradientEvaluation{"forward difference"};
    float scale{1.0f};
    float dvrFalloff{1.0f};
    float gaussianCurvatureFalloff{1.0f};
    float meanCurvatureFalloff{1.0f};
    float maxAbsCurvatureFalloff{1.0f};
    float normalLengthFalloff{1.0f};
    std::vector<Parameter> parameters;

    friend bool operator==(Data const &, Data const &) = default;
  };

  Function() = default;
  explicit Function(Data data);

  void onCreate();
  void onDestroy();

  [[nodiscard]] Data const &getData() const noexcept { return m_data; }
  [[nodiscard]] std::string const &getGLSLExpression() const noexcept {
    return m_exprGLSL;
  };
  [[nodiscard]] std::string getMathJaxEquation(float isoValue) const;
  [[nodiscard]] GLuint getThumbnailId() const noexcept { return m_thumbnailId; }
  [[nodiscard]] std::vector<Parameter> const &getParameters() const noexcept {
    return m_parameters;
  };
  bool setParameter(std::string_view name, float value);

  friend bool operator==(Function const &, Function const &) = default;

private:
  void extractParameters();
  void convertToGLSL();
  void convertToMathJax();

  Data m_data{};
  std::string m_exprGLSL{"p.x+p.y+p.z"};
  std::string m_exprMathJax{"x+y+z"};
  std::vector<Parameter> m_parameters;
  GLuint m_thumbnailId{};
};

#endif
