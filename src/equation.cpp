/**
 * @file equation.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#include <cctype>
#include <cstddef>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>

#include <cppitertools/itertools.hpp>
#include <fmt/core.h>

#include "equation.hpp"
#include "util.hpp"

void removeWhitespaces(std::string &str) {
  std::array const whitespaces{' ', '\f', '\n', '\r', '\t', '\v'};
  for (auto const &whitespace : whitespaces) {
    std::erase(str, whitespace);
  }
}

// Returns the position of the first pair of enclosing brackets starting from
// str[pos].
//
// Returns (npos, npos) if the brackets are not found or if there is a
// non-whitespace character from pos and the open bracket.
//
// Examples (using round brackets):
// str="f(x)"        pos=1 -> returns (1,2)
//       ^
// str="f   ((x)+1)" pos=1 -> returns (4,10)
//       ^
// str="f(x)"        pos=0 -> returns (npos, npos)
//      ^
std::pair<std::string::size_type, std::string::size_type>
getBracketsPos(std::string_view str, std::string::size_type pos,
               std::pair<char, char> brackets) {
  auto const failed{std::pair{std::string::npos, std::string::npos}};

  // Advance through whitespaces, from pos to the position of the open bracket.
  // If any non-whitespace character is found, return (npos, npos)
  auto startPos{pos};
  for (; startPos < str.length(); ++startPos) {
    auto const chr{str.at(startPos)};
    if (chr == brackets.first)
      break; // Open bracket found
    if (std::isspace(chr) == 0)
      return failed;
  }

  // Look for corresponding close bracket
  auto numNestedParens{0};
  if (!str.empty()) {
    for (auto endPos{startPos}; endPos < str.length() - 1; ++endPos) {
      auto const chr{str.at(endPos + 1)};
      if (chr == brackets.first)
        ++numNestedParens;
      if (chr == brackets.second) {
        if (numNestedParens > 0)
          --numNestedParens;
        else {
          ++endPos;
          return std::pair{startPos, endPos};
        }
      }
    }
  }
  return failed;
}

// Returns the position, in reverse order, of the first pair of enclosing
// brackets starting from str[pos].
//
// Returns (npos, npos) if the brackets are not found or if there is a
// non-whitespace character from pos and the open bracket.
//
// Examples using round brackets:
// str="(x)+"        pos=2 -> returns (2,0)
//        ^
// str="((x)+1)   +" pos=9 -> returns (6,0)
//               ^
// str="(x)+"        pos=3 -> returns (npos, npos)
//         ^
std::pair<std::string::size_type, std::string::size_type>
getBracketsPosReverse(std::string_view str, std::string::size_type pos,
                      std::pair<char, char> brackets) {
  auto const failed{std::pair{std::string::npos, std::string::npos}};

  // Reverse through whitespaces, from pos to the position of the close bracket.
  // If any non-whitespace character is found, return (npos, npos)
  auto startPos{pos};
  for (; startPos > 0; --startPos) {
    auto const chr{str.at(startPos)};
    if (chr == brackets.second)
      break; // Close bracket found
    if (std::isspace(chr) == 0)
      return failed;
  }
  if (startPos == 0)
    return failed;

  // Look for the corresponding open bracket
  auto numNestedParens{0};
  for (auto endPos{startPos}; endPos > 0; --endPos) {
    auto const chr{str.at(endPos - 1)};
    if (chr == brackets.second)
      ++numNestedParens;
    if (chr == brackets.first) {
      if (numNestedParens > 0)
        --numNestedParens;
      else {
        --endPos;
        return std::pair{startPos, endPos};
      }
    }
  }
  return failed;
}

// Returns the sizes of the left and right operands of the operator at str[pos]
std::pair<std::string::size_type, std::string::size_type>
getOperandSizes(std::string str, std::string::size_type pos) {
  auto sizes{std::pair{0UL, 0UL}};

  auto notWhitespace{[](char chr) { return std::isspace(chr) == 0; }};
  auto notName{[](char chr) { return std::isalnum(chr) == 0 && chr != '_'; }};
  auto notNameOrNumber{[&](char chr) {
    return notName(chr) && chr != '.' && std::isdigit(chr) == 0;
  }};

  // Compute size of right operand

  // Go to first non-whitespace character
  if (auto itr{std::find_if(str.begin() + gsl::narrow<long>(pos) + 1, str.end(),
                            notWhitespace)};
      itr != str.end()) {
    if (*itr == '(') {
      // Open bracket
      auto const startPos{gsl::narrow<unsigned long>(itr - str.begin())};
      auto const bracketPos{getBracketsPos(str, startPos, {'(', ')'})};
      if (bracketPos.first != std::string_view::npos) {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267) // Safe conversion from size_t to unsigned long
#endif
        sizes.second = bracketPos.second - pos;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
      }
    } else {
      if (*itr == '-' || *itr == '+') {
        itr = std::next(itr);
      }
      itr = std::find_if(itr, str.end(), notNameOrNumber);
      auto const endPos{itr - str.begin()};
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267) // Safe conversion from size_t to unsigned long
#endif
      sizes.second = gsl::narrow<unsigned long>(endPos) - pos - 1;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    }
  }

  // Compute size of left operand

  // Go to first non-whitespace character
  if (auto itr{std::find_if(str.rbegin() + gsl::narrow<long>(str.size() - pos),
                            str.rend(), notWhitespace)};
      itr != str.rend()) {
    if (*itr == ')') {
      // Close bracket
      auto const startPos{std::distance(itr, str.rend()) - 1};
      auto const bracketPos{getBracketsPosReverse(
          str, gsl::narrow<unsigned long>(startPos), {'(', ')'})};
      if (bracketPos.first != std::string_view::npos) {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267) // Safe conversion from size_t to unsigned long
#endif
        sizes.first = pos - bracketPos.second;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
      }
    } else {
      itr = std::find_if(itr, str.rend(), notNameOrNumber);
      auto const endPos{std::distance(itr, str.rend()) - 1};
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267) // Safe conversion from size_t to unsigned long
#endif
      sizes.first = pos - gsl::narrow_cast<unsigned long>(endPos) - 1;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    }
  }

  return sizes;
}

Equation::Equation(LoadedData data) : m_loadedData(std::move(data)) {
  replaceAll(m_loadedData.expression, "\\n", "\n");
  convertToGLSL();
  convertToMathJax();
}

bool Equation::setParameter(std::string_view name, float value) {
  if (auto itr{std::find_if(
          m_parameters.begin(), m_parameters.end(),
          [&name](auto parameter) { return parameter.name == name; })};
      itr != m_parameters.end()) {
    (*itr).value = value;
    return true;
  }
  return false;
}

void Equation::onCreate() {
  auto const path{abcg::Application::getAssetsPath()};
  if (!m_loadedData.thumbnail.empty()) {
    m_thumbnailId =
        abcg::loadOpenGLTexture(path + m_loadedData.thumbnail, false, false);
  }
}

void Equation::onDestroy() { abcg::glDeleteTextures(1, &m_thumbnailId); }

// Get all names from the given expression.
// A name is a string that starts with '_' or an alphabet letter, and is
// composed of the characters [A-Za-z0-9_].
std::set<std::string> getNames(std::string_view expression) {
  std::set<std::string> names;
  std::string str{};
  for (auto const chr : expression) {
    if (str.empty()) {
      if (std::isalpha(chr) != 0 || chr == '_') {
        str.push_back(chr);
      }
    } else {
      if (std::isalnum(chr) != 0 || chr == '_') {
        str.push_back(chr);
      } else {
        names.insert(str);
        str.clear();
      }
    }
  }
  if (!str.empty()) {
    names.insert(str);
    str.clear();
  }
  return names;
}

void Equation::extractParameters() {
  std::set<std::string> parameters{getNames(m_loadedData.expression)};

  // Remove reserved names
  std::array const reservedNames{"x",
                                 "y",
                                 "z",
                                 "pi",
                                 "kBoundRadius",
                                 "kBoundRadiusSquared",
                                 "fragPosition",
                                 "outColor",
                                 "uCamera",
                                 "uShading",
                                 "uTransform",
                                 "uParams",
                                 "uIsoValue"};
  for (auto const &name : reservedNames) {
    parameters.erase(name);
  }

  // Remove function names found in str
  auto removeFunctionNames{[&parameters](std::string str) {
    static std::regex const regex{R"del((([a-zA-Z_])\w*)\s*\()del",
                                  std::regex_constants::optimize};
    std::vector<std::string> customFunctionNames{};
    for (std::smatch match; std::regex_search(str, match, regex);
         str = str.substr(
             gsl::narrow<unsigned long>(match.position() + match.length()))) {
      auto matchString{match[1].str()};
      customFunctionNames.push_back(matchString);
    }
    for (auto const &name : customFunctionNames) {
      parameters.erase(name);
    }
  }};
  removeFunctionNames(m_loadedData.codeGlobal);
  removeFunctionNames(m_loadedData.expression);

  // Check if the character at str[pos] is enclosed by curly brackets
  auto insideCurlyBrackets{[](std::string_view str, std::size_t pos) {
    int groups{};
    for (auto const idx : iter::range(pos, str.length())) {
      auto chr{str.at(idx)};
      if (chr == '{')
        ++groups;
      if (chr == '}')
        --groups;
    }
    return groups != 0;
  }};

  // Remove names of constant variables found in global scope
  {
    std::vector<std::string> customConstantNames{};
    static std::regex const regex{
        R"del(const\s+[A-Za-z_]\w*\s+([A-Za-z_]\w*)\s*=)del",
        std::regex_constants::optimize};
    std::string str{m_loadedData.codeGlobal};
    for (std::smatch sm; std::regex_search(str, sm, regex);
         str = str.substr(
             gsl::narrow<unsigned long>(sm.position() + sm.length()))) {
      auto match{sm[1].str()};

      if (!insideCurlyBrackets(str,
                               gsl::narrow<unsigned long>(sm.position()))) {
        customConstantNames.push_back(match);
      }
    }
    for (auto const &name : customConstantNames) {
      parameters.erase(name);
    }
  }

  // Remove names of variables found in local scope
  {
    std::vector<std::string> customVariableNames{};
    static std::regex const regex{
        R"del(\b[A-Za-z_]\w*\s+([A-Za-z_]\w*)\s*=)del",
        std::regex_constants::optimize};
    std::string str{m_loadedData.codeLocal};
    for (std::smatch sm; std::regex_search(str, sm, regex);
         str = str.substr(
             gsl::narrow<unsigned long>(sm.position() + sm.length()))) {
      auto match{sm[1].str()};
      if (!insideCurlyBrackets(str,
                               gsl::narrow<unsigned long>(sm.position()))) {
        customVariableNames.push_back(match);
      }
    }
    for (auto const &name : customVariableNames) {
      parameters.erase(name);
    }
  }

  // Populate vector of Parameter
  for (auto const &parameter : parameters) {
    m_parameters.push_back({parameter, 1.0f});
  }

  // Set parameters values from loaded data
  for (auto const &parameter : m_loadedData.parameters) {
    setParameter(parameter.name, parameter.value);
  }
}

void Equation::convertToMathJax() {
  std::string result{m_loadedData.expression};

  std::array const greekLetters{"alpha",  "beta", "gamma", "delta", "epsilon",
                                "zeta",   "eta",  "theta", "iota",  "kappa",
                                "lambda", "mu",   "nu",    "xi",    "omikron",
                                "pi",     "rho",  "sigma", "tau",   "upsilon",
                                "phi",    "chi",  "psi",   "omega"};

  // Single-token arguments (variables, parameters and constants)
  std::vector<std::string> singleTokenArgs{"0", "1", "2", "3", "4", "5", "6",
                                           "7", "8", "9", "x", "y", "z"};
  if (m_parameters.empty())
    extractParameters();
  for (auto const &param : m_parameters) {
    singleTokenArgs.push_back(param.name);
  }
  for (auto const &greekLetter : greekLetters) {
    singleTokenArgs.emplace_back(greekLetter);
  }

  // Replace "name(x)" with "name{x}" where x is a single-token argument
  auto reformatCallWithSingleToken{
      [&singleTokenArgs](std::string &str, std::string_view name) {
        for (auto const &variable : singleTokenArgs) {
          replaceAll(str, fmt::format("{}({})", name, variable),
                     fmt::format("{}{{{}}}", name, variable));
        }
      }};

  // Remove "\" (backslash)
  std::erase(result, '\\');

  // Replace [,] with \left[,\right]
  replaceAll(result, "[", "\\left[");
  replaceAll(result, "]", "\\right]");

  // Remove whitespaces except line feeds
  for (std::array const whitespaces{' ', '\f', '\r', '\t', '\v'};
       auto const &whitespace : whitespaces) {
    std::erase(result, whitespace);
  }

  // Replace "\n" with "\\\\&" (newline)
  replaceAll(result, "\n", "\\\\&");

  // Replace "**" with "^" (exponentiation)
  replaceAll(result, "**", "^");

  // Replace "name" with "\name "
  for (auto const &name : greekLetters) {
    auto const with{std::string{"\\"} + name + ' '};
    replaceAll(result, name, with, true);
  }

  // From inout[pos], replace the first group of srcBrackets with dstBrackets
  std::pair const srcBrackets{'(', ')'};
  std::pair dstBrackets{'{', '}'};
  auto replaceBrackets{[&srcBrackets, &dstBrackets](auto &inout, auto pos) {
    if (auto const bracketsPos{getBracketsPos(inout, pos, srcBrackets)};
        bracketsPos.first != inout.npos) {
      inout.at(bracketsPos.first) = dstBrackets.first;
      inout.at(bracketsPos.second) = dstBrackets.second;
    }
  }};

  // Replace "exp" with "e^"
  replaceAll(result, "exp", "e^", true);

  // Replace "exp2" with "2^"
  replaceAll(result, "exp2", "2^", true);

  // Replace "^expr" with "^{expr}"
  {
    std::size_t pos{};
    std::smatch match;
    // Capture "name" from "^name(" match
    static std::regex const regex(R"del(\^([a-zA-Z_]*[a-zA-Z0-9_.]*\s*))del",
                                  std::regex_constants::optimize);

    for (std::string ns{result}; std::regex_search(ns, match, regex);) {
      auto const posAfterName{match.position(1) + match.length(1) - 1};
      auto bracketPos{getBracketsPos(
          ns, gsl::narrow<unsigned long>(posAfterName + 1), {'(', ')'})};
      std::string bracketedArgs{};
      if (bracketPos.first != std::string::npos &&
          bracketPos.second != std::string::npos) {
        bracketedArgs = ns.substr(bracketPos.first,
                                  bracketPos.second - bracketPos.first + 1);
      }
      auto const what{match[1].str() + bracketedArgs}; // "name" + "(...)"
      auto const with{"{" + what + "}"};               // "{name(...)}"
      result.replace(pos + gsl::narrow<unsigned long>(match.position(1)),
                     what.length(), with.data(), with.length());
      ns.replace(gsl::narrow<unsigned long>(match.position(1)), what.length(),
                 with.data(), with.length());

      auto const advancePos{
          gsl::narrow<unsigned long>(match.position(1) + match.length(1))};
      pos += advancePos;
      ns = ns.substr(advancePos);
    }
  }

  for (std::array const names{"asin", "acos", "atan", "sinh", "cosh", "tanh",
                              "asinh", "acosh", "atanh", "sin", "cos", "tan",
                              "min", "max"};
       auto const &name : names) {
    auto const with{std::string{"\\"} + name};
    // Replace "name" with "\name"
    replaceAll(result, name, with, true);
    // f(x) to f{x}
    reformatCallWithSingleToken(result, with);
  }

  // Replace "log" with "\ln"
  replaceAll(result, "log", "\\ln", true);
  reformatCallWithSingleToken(result, "\\ln"); // ln(x) to ln{x}

  // Replace "log2" with "\log_2"
  replaceAll(result, "log2", "\\log_2", true);
  reformatCallWithSingleToken(result, "\\log_2"); // log_2(x) to log_2{x}

  // Replace "sign" with "\sgn"
  replaceAll(result, "sign", "\\sgn", true);
  reformatCallWithSingleToken(result, "\\sgn"); // sgn(x) to sgn{x}

  // Replace "sqrt(...)" with "\sqrt{...}"
  replaceAllAndInvoke(result, "sqrt", "\\sqrt", replaceBrackets, true);

  // Replace "abs(...)" with "|...|"
  dstBrackets = std::pair{'|', '|'};
  replaceAllAndInvoke(result, "abs", "", replaceBrackets, true);

  // Replace "floor(...)" with "@...#"
  dstBrackets = std::pair{'@', '#'};
  replaceAllAndInvoke(result, "floor", "", replaceBrackets, true);
  // Replace "@" with "\lfloor"
  replaceAll(result, "@", "\\lfloor", false);
  // Replace "#" with "\rfloor"
  replaceAll(result, "#", "\\rfloor", false);
  // Replace "ceil(...)" with "@...#"
  replaceAllAndInvoke(result, "ceil", "", replaceBrackets, true);
  // Replace "@" with "\lceil"
  replaceAll(result, "@", "\\lceil", false);
  // Replace "#" with "\rceil"
  replaceAll(result, "#", "\\rceil", false);

  // Remove "*" (multiplication)
  std::erase(result, '*');

  // Remove "(x,y,z)"
  replaceAll(result, "(x,y,z)", "", false);

  // Replace "{(...)}" with "{...}"
  {
    std::smatch match;
    // Match "{(expr)}"
    static std::regex const regex(R"del(\{(\(.+\))\})del",
                                  std::regex_constants::optimize);
    for (std::string ns{result}; std::regex_search(ns, match, regex);
         ns = result) {
      auto const posOpenParens{
          gsl::narrow<unsigned long>(match.position() + 1)};
      auto bracketPos{getBracketsPos(ns, posOpenParens, {'(', ')'})};
      // Check if the position of the closing parenthesis matches the ")}" at
      // the end of the expression
      if (bracketPos.second ==
          gsl::narrow<std::size_t>(match.position() + match.length() - 2)) {
        auto const expr{ns.substr(bracketPos.first + 1,
                                  bracketPos.second - bracketPos.first - 1)};
        auto const &what{match.str()};     // "{(expr)}"
        auto const with{"{" + expr + "}"}; // "{expr}"
        result.replace(gsl::narrow<unsigned long>(match.position()),
                       what.length(), with.data(), with.length());
      }
    }
  }

  m_exprMathJax = result;
}

void Equation::convertToGLSL() {
  std::array const variables{"x", "y", "z"};

  std::string result{m_loadedData.expression};
  removeWhitespaces(result);

  // Replace [] with ()
  replaceAll(result, "[", "(");
  replaceAll(result, "]", ")");

  // Parenthesize function calls: f(...) to (f(...))
  {
    std::smatch match;
    std::unordered_set<std::string> functionCalls;
    // Match "name("
    static std::regex const regex(R"del(\b[a-zA-Z_]*[a-zA-Z0-9_]*\s*\()del",
                                  std::regex_constants::optimize);
    for (std::string ns{result}; std::regex_search(ns, match, regex);
         ns = ns.substr(
             gsl::narrow<unsigned long>(match.position() + match.length()))) {
      auto const startPos{match.position() + match.length() - 1};
      auto const bracketPos{
          getBracketsPos(ns, gsl::narrow<unsigned long>(startPos), {'(', ')'})};
      auto const callArgs{ns.substr(bracketPos.first + 1,
                                    bracketPos.second - bracketPos.first)};
      functionCalls.emplace(match.str() + callArgs); // "name(" + "...)"
    }
    for (auto const &fcall : functionCalls) {
      replaceAll(result, fcall, fmt::format("({})", fcall));
    }
  }

  // Replace x^y with mpowy(x) or mpow(x,y)
  auto const maxPowerByMultiplication{16.0f};
  for (std::string::size_type idx{}; idx < result.length(); ++idx) {
    if (result[idx] == '^') {
      auto const operandSizes{getOperandSizes(result, idx)};
      auto const exprSize{operandSizes.first + operandSizes.second + 1};
      auto const exprFirstPos{idx - operandSizes.first};
      auto const leftOperand{result.substr(exprFirstPos, operandSizes.first)};
      auto const rightOperand{result.substr(
          exprFirstPos + operandSizes.first + 1, operandSizes.second)};

      auto isIntegral{false};
      char *end{};
      auto power{std::strtof(rightOperand.c_str(), &end)};
      if (power > 0.0f && power <= maxPowerByMultiplication) {
        auto integralPart{0.0f};
        isIntegral = std::modf(power, &integralPart) == 0.0f;
      }
      std::string with{};
      if (isIntegral) {
        if (power == 1.0f) {
          with = fmt::format("({})", leftOperand);
        } else {
          with = fmt::format("mpow{:.0f}({})", power, leftOperand);
        }
      } else {
        with = fmt::format("mpow({},{})", leftOperand, rightOperand);
      }
      result.replace(exprFirstPos, exprSize, with);
      idx = exprFirstPos;
    }
  }

  // Replace x with p.x, y with p.y, etc
  for (auto const *chr : variables) {
    replaceAll(result, chr, fmt::format("p.{}", chr), true);
  }

  // Format numbers as decimals
  {
    std::smatch match;
    std::size_t pos{};
    // Match either integers (e.g. 42) or floats (e.g. .42 or 4.2)
    static std::regex const regex(R"del((\.\d+\.?\d*)|\b(\d+\.?\d*))del",
                                  std::regex_constants::optimize);
    for (std::string ns{result}; std::regex_search(ns, match, regex);
         ns = ns.substr(
             gsl::narrow<unsigned long>(match.position() + match.length()))) {
      auto const number{std::stod(match.str())};
      auto integralPart{0.0};
      auto const fractionalPart{std::modf(number, &integralPart)};
      auto const formattedString{fractionalPart == 0.0
                                     ? fmt::format("{:.1f}", number)
                                     : fmt::format("{:.12g}", number)};
      result.replace(pos + gsl::narrow<unsigned long>(match.position()),
                     gsl::narrow<unsigned long>(match.length()),
                     formattedString);
      pos += formattedString.length() +
             gsl::narrow<unsigned long>(match.position());
    }
  }

  m_exprGLSL = result;
}

[[nodiscard]] std::string Equation::getMathJaxExpression(float isoValue) const {
  // Append "= isoValue"
  return fmt::format("{}={:.3g}", m_exprMathJax, isoValue);
};

std::vector<Equation> Equation::loadCatalogue(std::string_view filename) {
  std::vector<Equation> result;

  std::stringstream sstream;
  if (std::ifstream stream(filename.data()); stream) {
    sstream << stream.rdbuf();
    stream.close();
  } else {
    throw abcg::RuntimeError(fmt::format("Failed to load {}", filename));
  }

  auto trimLeftWhitespaces{[](std::string str) {
    static auto notWhitespace{[](auto chr) { return std::isspace(chr) == 0; }};
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), notWhitespace));
    return str;
  }};

  // Each data entry is composed of a sequence of attributes and values.
  // The expected file layout is as follows ($ denotes an attribute's value):
  //
  // name $name              // Equation name (string)
  // thumb $thumb            // Thumbnail filename (string)
  // [bound_shape $type]     // Bounding shape (0 or 1)
  // [bound_radius $radius]  // Bounding radius (positive float)
  // [raymarch_method $type] // Ray marching method (0 or 1)
  // [raymarch_steps $steps] // Ray marching steps (positive integer)
  // [raymarch_root_test $t] // Ray marching root test (0 or 1)
  // [cam_dist $dist]        // Distance from camera (positive float)
  // [colormap_scale $value] // Colormap scaling factor (positive float)
  // [param $name $value]    // Parameter name (string) and value (number)
  // [param ...   ...   ]
  // [code_local $code]      // Local scope code (string; can span many lines)
  // [...             ]
  // [code_global $code]     // Global scope code (string; can span many lines)
  // [...              ]
  // [comment $comment]      // Comment (string; can span many lines)
  // [...             ]
  // expr $expr              // Expression (string; can span many lines)
  // ...
  //                         // Blank line or EOF indicates end of entry
  //
  // 'name', 'thumb' and 'expr' are required attributes.
  // The values of 'code_local', 'code_global', 'comment' and 'expr' can span
  // multiple lines.
  // 'param' can appear multiple times as long as the names are unique.

  LoadedData loadedData{};

  enum class Attribute {
    Name,
    Thumb,
    Param,
    Expr,
    CodeLocal,
    CodeGlobal,
    Comment,
    BoundShape,
    BoundRadius,
    RayMarchMethod,
    RayMarchSteps,
    RayMarchRootTest,
    CamDist,
    ColormapScale,
    None,
    Count
  };

  auto const attributeCount{gsl::narrow<std::size_t>(Attribute::Count) - 1};
  using ParserFunction = void(std::string const &, LoadedData &);
  std::array<std::tuple<Attribute, std::string, ParserFunction *>,
             attributeCount> const attribParsers{
      std::tuple{Attribute::Name, "name",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.name = attribValue;
                 }},
      std::tuple{Attribute::Thumb, "thumb",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.thumbnail = attribValue;
                 }},
      std::tuple{Attribute::Param, "param",
                 [](std::string const &attribValue, LoadedData &data) {
                   std::string str{attribValue};
                   static std::regex const regex(
                       R"del(\s*[A-Za-z_\d.+-]*)del",
                       std::regex_constants::optimize);
                   std::smatch match{};
                   if (std::regex_search(str, match, regex)) {
                     auto const name{match.str()}; // Parameter name found
                     str = str.substr(gsl::narrow<unsigned long>(
                         match.position() + match.length()));
                     if (std::regex_search(str, match, regex)) {
                       auto const value{
                           std::stof(match.str())}; // Parameter value found
                       data.parameters.push_back({name, value});
                     }
                   }
                 }},
      std::tuple{Attribute::Expr, "expr",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.expression = attribValue;
                 }},
      std::tuple{Attribute::CodeLocal, "code_local",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.codeLocal = attribValue;
                 }},
      std::tuple{Attribute::CodeGlobal, "code_global",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.codeGlobal = attribValue;
                 }},
      std::tuple{Attribute::Comment, "comment",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.comment = attribValue;
                 }},
      std::tuple{Attribute::BoundShape, "bound_shape",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.boundShape = std::stoi(attribValue);
                 }},
      std::tuple{Attribute::BoundRadius, "bound_radius",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.boundRadius = std::stof(attribValue);
                 }},
      std::tuple{Attribute::RayMarchMethod, "raymarch_method",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.rayMarchMethod = std::stoi(attribValue);
                 }},
      std::tuple{Attribute::RayMarchSteps, "raymarch_steps",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.rayMarchSteps = std::stoi(attribValue);
                 }},
      std::tuple{Attribute::RayMarchRootTest, "raymarch_root_test",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.rayMarchRootTest = std::stoi(attribValue);
                 }},
      std::tuple{Attribute::CamDist, "cam_dist",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.camDist = std::stof(attribValue);
                 }},
      std::tuple{Attribute::ColormapScale, "colormap_scale",
                 [](std::string const &attribValue, LoadedData &data) {
                   data.colormapScale = std::stof(attribValue);
                 }}};

  Attribute currentAttribute{Attribute::None};

  for (std::string line{}; std::getline(sstream, line);) {
    if (line.empty()) {
      result.emplace_back(loadedData);
      loadedData = {};
      continue;
    }

    auto newAttributeFound{false};
    for (auto &&[attrib, attribName, parserFunction] : attribParsers) {
      if (line.starts_with(attribName)) {
        currentAttribute = attrib;
        auto attribValue{trimLeftWhitespaces(line.substr(attribName.length()))};
        parserFunction(attribValue, loadedData);
        newAttributeFound = true;
        break;
      }
    }
    if (newAttributeFound)
      continue;

    switch (currentAttribute) {
    case Attribute::CodeLocal:
      loadedData.codeLocal += '\n' + line;
      break;
    case Attribute::CodeGlobal:
      loadedData.codeGlobal += '\n' + line;
      break;
    case Attribute::Comment:
      loadedData.comment += "\\\\" + line;
      break;
    case Attribute::Expr:
      loadedData.expression += line;
      break;
    default:
      break;
    }
  }

  if (!loadedData.expression.empty()) {
    result.emplace_back(loadedData);
  }

  return result;
}
