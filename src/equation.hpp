/**
 * @file equation.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#ifndef EQUATION_HPP_
#define EQUATION_HPP_

#include "abcgOpenGL.hpp"

/*
  static RE2 const regexName{R"del(([A-Za-z_]\w*))del"};
  assert(regexName.ok()); // NOLINT

void insertMatches(RE2 const &regex, re2::StringPiece str,
                   std::set<std::string> &output) {

  for (std::string match; RE2::FindAndConsume(&str, regex, &match);) {
    output.insert(match);
  }

*/

#include <string>
#include <vector>

class Equation {
public:
  struct Parameter {
    std::string name;
    float value{};
  };

  struct LoadedData {
    std::string name;
    std::string thumbnail;
    std::string expression;
    std::string codeLocal;
    std::string codeGlobal;
    std::string comment;
    int boundShape{};
    float boundRadius{2.5f};
    int rayMarchMethod{};
    int rayMarchSteps{150};
    int rayMarchRootTest{};
    float camDist{10.0f};
    float colormapScale{0.75f};
    std::vector<Parameter> parameters;
  };

  Equation() = default;
  explicit Equation(LoadedData data);

  void onCreate();
  void onDestroy();

  [[nodiscard]] LoadedData const &getLoadedData() const { return m_loadedData; }
  [[nodiscard]] std::string getGLSLExpression() const { return m_exprGLSL; };
  [[nodiscard]] std::string getMathJaxExpression(float isoValue) const;
  [[nodiscard]] GLuint getThumbnailId() const noexcept { return m_thumbnailId; }
  [[nodiscard]] std::vector<Parameter> const &getParameters() const {
    return m_parameters;
  };
  bool setParameter(std::string_view name, float value);

  [[nodiscard]] static std::vector<Equation>
  loadCatalogue(std::string_view filename);

private:
  void extractParameters();
  void convertToGLSL();
  void convertToMathJax();

  LoadedData m_loadedData{};

  std::string m_exprGLSL;
  std::string m_exprMathJax;
  std::vector<Parameter> m_parameters;
  GLuint m_thumbnailId{};
};

#endif
