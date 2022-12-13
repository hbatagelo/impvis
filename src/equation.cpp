/**
 * @file equation.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#include <re2/re2.h>
#include <string_view>

#include "equation.hpp"
#include "util.hpp"

// Returns the position of the first pair of enclosing braces starting from
// str[pos].
//
// Returns (npos, npos) if the braces are not found or if there is a
// non-whitespace character from pos and the opening brace.
//
// Examples using parens:
// str="f(x)"        pos=1 -> returns (1,3)
//       ^
// str="f   ((x)+1)" pos=1 -> returns (4,10)
//       ^
// str="f(x)"        pos=0 -> returns (npos, npos)
//      ^
std::pair<std::string::size_type, std::string::size_type>
getBracesPos(std::string_view str, std::string::size_type pos,
             std::pair<char, char> braces) {
  auto const failed{std::pair{std::string::npos, std::string::npos}};

  // Advance through whitespaces, from pos to the position of the opening brace.
  // If any non-whitespace character is found, return (npos, npos)
  auto startPos{pos};
  for (; startPos < str.length(); ++startPos) {
    auto const chr{str.at(startPos)};
    if (chr == braces.first)
      break; // Opening brace found
    if (std::isspace(chr) == 0)
      return failed;
  }

  // Look for corresponding closing brace
  auto numNestedParens{0};
  if (!str.empty()) {
    for (auto endPos{startPos}; endPos < str.length() - 1; ++endPos) {
      auto const chr{str.at(endPos + 1)};
      if (chr == braces.first)
        ++numNestedParens;
      if (chr == braces.second) {
        if (numNestedParens > 0)
          --numNestedParens;
        else {
          ++endPos;
          assert(startPos <= endPos); // NOLINT
          return std::pair{startPos, endPos};
        }
      }
    }
  }

  return failed;
}

// Returns the position, in reverse order, of the first pair of enclosing
// braces starting from str[pos].
//
// Returns (npos, npos) if the braces are not found or if there is a
// non-whitespace character from pos and the opening brace.
//
// Examples using parens:
// str="(x)+"        pos=2 -> returns (2,0)
//        ^
// str="((x)+1)   +" pos=9 -> returns (6,0)
//               ^
// str="(x)+"        pos=3 -> returns (npos, npos)
//         ^
std::pair<std::string::size_type, std::string::size_type>
getBracesPosReverse(std::string_view str, std::string::size_type pos,
                    std::pair<char, char> braces) {
  auto const failed{std::pair{std::string::npos, std::string::npos}};

  // Reverse through whitespaces, from pos to the position of the closing brace.
  // If any non-whitespace character is found, return (npos, npos)
  auto startPos{pos};
  for (; startPos > 0; --startPos) {
    auto const chr{str.at(startPos)};
    if (chr == braces.second)
      break; // Closing brace found
    if (std::isspace(chr) == 0)
      return failed;
  }
  if (startPos == 0)
    return failed;

  // Look for corresponding opening brace
  auto numNestedParens{0};
  for (auto endPos{startPos}; endPos > 0; --endPos) {
    auto const chr{str.at(endPos - 1)};
    if (chr == braces.second)
      ++numNestedParens;
    if (chr == braces.first) {
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
std::pair<std::size_t, std::size_t> getOperandSizes(std::string str,
                                                    std::size_t pos) {
  assert(pos < str.size()); // NOLINT

  auto sizes{std::pair{std::size_t{}, std::size_t{}}};

  auto notWhitespace{[](char chr) { return std::isspace(chr) == 0; }};
  auto notName{[](char chr) { return std::isalnum(chr) == 0 && chr != '_'; }};
  auto notNameOrNumber{[&](char chr) {
    return notName(chr) && chr != '.' && std::isdigit(chr) == 0;
  }};

  // Compute size of right operand

  // Go to first non-whitespace character
  if (auto itr{std::find_if(str.begin() + gsl::narrow<std::ptrdiff_t>(pos) + 1,
                            str.end(), notWhitespace)};
      itr != str.end()) {
    if (*itr == '(') {
      // Opening brace
      auto const startPos{std::distance(str.begin(), itr)};
      auto const bracePos{
          getBracesPos(str, gsl::narrow<std::size_t>(startPos), {'(', ')'})};
      if (bracePos.first != std::string_view::npos) {
        sizes.second = bracePos.second - pos;
      }
    } else {
      if (*itr == '-' || *itr == '+') {
        itr = std::next(itr);
      }
      itr = std::find_if(itr, str.end(), notNameOrNumber);
      auto const endPos{std::distance(str.begin(), itr)};
      auto const opSize{endPos - gsl::narrow<std::ptrdiff_t>(pos) - 1};
      sizes.second = gsl::narrow<std::size_t>(opSize);
    }
  }

  // Compute size of left operand

  // Go to first non-whitespace character
  if (auto itr{std::find_if(str.rbegin() +
                                gsl::narrow<std::ptrdiff_t>(str.size() - pos),
                            str.rend(), notWhitespace)};
      itr != str.rend()) {
    if (*itr == ')') {
      // Closing brace
      auto const startPos{std::distance(itr, str.rend()) - 1};
      auto const bracePos{getBracesPosReverse(
          str, gsl::narrow<std::size_t>(startPos), {'(', ')'})};
      if (bracePos.first != std::string_view::npos) {
        sizes.first = pos - bracePos.second;
      }
    } else {
      itr = std::find_if(itr, str.rend(), notNameOrNumber);
      auto const endPos{std::distance(itr, str.rend()) - 1};
      auto const opSize{gsl::narrow<std::ptrdiff_t>(pos) - endPos - 1};
      sizes.first = gsl::narrow<std::size_t>(opSize);
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

// Insert matches to output set
void insertMatches(RE2 const &regex, re2::StringPiece str,
                   std::set<std::string> &output) {
  for (std::string match; RE2::FindAndConsume(&str, regex, &match);) {
    output.insert(match);
  }
}

// Remove matches from output set
void removeMatches(RE2 const &regex, re2::StringPiece str,
                   std::set<std::string> &output) {
  for (std::string match; RE2::FindAndConsume(&str, regex, &match);) {
    output.erase(match);
  }
}

// Remove matches that are not enclosed by a block scope
void removeMatchesInSameScope(RE2 const &regex, re2::StringPiece str,
                              std::set<std::string> &output) {
  // Helper lambda that checks if the character at `str[pos]` is enclosed
  // by curly braces (assuming no dangling braces)
  auto insideCurlyBraces{[](std::string_view view, std::size_t pos) {
    int groups{};
    for (auto const idx : iter::range(pos, view.length())) {
      auto const chr{view.at(idx)};
      if (chr == '{')
        ++groups;
      if (chr == '}')
        --groups;
    }
    return groups != 0;
  }};

  for (re2::StringPiece match; RE2::PartialMatch(str, regex, &match);) {
    auto const matchPosition{
        gsl::narrow<std::size_t>(match.data() - str.begin())};
    if (!insideCurlyBraces(str, matchPosition)) {
      output.erase(match.as_string());
    }
    str.remove_prefix(matchPosition + match.size());
  }
}

void Equation::extractParameters() {
  std::set<std::string> parameters;

  // Insert names
  static RE2 const regexName{R"del(([A-Za-z_]\w*))del"};
  assert(regexName.ok()); // NOLINT
  insertMatches(regexName, m_loadedData.expression, parameters);

  // Remove reserved names
  static std::array const reservedNames{"x",
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

  // Remove function names
  static RE2 const regexFunctionName{R"del((([a-zA-Z_])\w*)\s*\()del"};
  assert(regexFunctionName.ok()); // NOLINT
  removeMatches(regexFunctionName, m_loadedData.codeGlobal, parameters);
  removeMatches(regexFunctionName, m_loadedData.expression, parameters);

  // Remove global constant variables
  static RE2 const regexConstantVariable{
      R"del(const\s+[A-Za-z_]\w*\s+([A-Za-z_]\w*)\s*=)del"};
  assert(regexConstantVariable.ok()); // NOLINT
  removeMatchesInSameScope(regexConstantVariable, m_loadedData.codeGlobal,
                           parameters);

  // Remove local variables
  static RE2 const regexVariable{
      R"del(\b[A-Za-z_]\w*\s+([A-Za-z_]\w*)\s*=)del"};
  assert(regexVariable.ok()); // NOLINT
  removeMatchesInSameScope(regexVariable, m_loadedData.codeLocal, parameters);

  // Populate vector of parameters
  for (auto const &parameter : parameters) {
    m_parameters.push_back({parameter, 1.0f});
  }

  // Set parameters values from loaded data
  for (auto const &parameter : m_loadedData.parameters) {
    setParameter(parameter.name, parameter.value);
  }
}

void encloseMatchesInCurlyBraces(std::string &str, RE2 const &regex) {
  std::size_t pos{};
  re2::StringPiece match;

  for (std::string ns{str}; RE2::PartialMatch(ns, regex, &match);) {
    auto const matchPosition{
        gsl::narrow<std::size_t>(match.data() - ns.c_str())};
    auto const posAfterName{matchPosition + match.size()};
    auto const bracePos{getBracesPos(ns, posAfterName, {'(', ')'})};

    std::string bracetedArgs{};
    if (bracePos.first != std::string::npos &&
        bracePos.second != std::string::npos) {
      assert(bracePos.second >= bracePos.first); // NOLINT
      bracetedArgs =
          ns.substr(bracePos.first, 1 + bracePos.second - bracePos.first);
    }

    auto const what{match.as_string() + bracetedArgs}; // "name" + "(...)"
    auto const with{"{" + what + "}"};                 // "{name(...)}"
    str.replace(pos + matchPosition, what.length(), with.data(), with.length());
    ns.replace(matchPosition, what.length(), with.data(), with.length());

    auto const advancePos{matchPosition + match.size()};
    pos += advancePos;
    ns = ns.substr(advancePos);
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

  // From inout[pos], replace the first group of srcBraces with dstBraces
  std::pair const srcBraces{'(', ')'};
  std::pair dstBraces{'{', '}'};
  auto replaceBraces{[&srcBraces, &dstBraces](auto &inout, auto pos) {
    if (auto const bracesPos{getBracesPos(inout, pos, srcBraces)};
        bracesPos.first != inout.npos) {
      inout.at(bracesPos.first) = dstBraces.first;
      inout.at(bracesPos.second) = dstBraces.second;
    }
  }};

  // Replace "^expr" with "^{expr}"
  static RE2 const regexExponent{R"del(\^([a-zA-Z_]*[a-zA-Z0-9_.]*\s*))del"};
  assert(regexExponent.ok()); // NOLINT
  encloseMatchesInCurlyBraces(result, regexExponent);

  // Replace "name(x)" with "name{x}" where x is a single-token argument
  auto reformatCallWithSingleToken{
      [&singleTokenArgs](std::string &str, std::string_view name) {
        for (auto const &variable : singleTokenArgs) {
          replaceAll(str, fmt::format("{}({})", name, variable),
                     fmt::format("{}{{{}}}", name, variable));
        }
      }};

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

  // Replace "exp(...)" with "e^{...}"
  replaceAllAndInvoke(result, "exp", "e^", replaceBraces, true);

  // Replace "exp2(...)" with "2^{...}"
  replaceAllAndInvoke(result, "exp2", "2^", replaceBraces, true);

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
  replaceAllAndInvoke(result, "sqrt", "\\sqrt", replaceBraces, true);

  // Replace "abs(...)" with "|...|"
  dstBraces = std::pair{'|', '|'};
  replaceAllAndInvoke(result, "abs", "", replaceBraces, true);

  // Replace "floor(...)" with "@...#"
  dstBraces = std::pair{'@', '#'};
  replaceAllAndInvoke(result, "floor", "", replaceBraces, true);

  // Replace "@" with "\lfloor"
  replaceAll(result, "@", "\\lfloor", false);

  // Replace "#" with "\rfloor"
  replaceAll(result, "#", "\\rfloor", false);

  // Replace "ceil(...)" with "@...#"
  replaceAllAndInvoke(result, "ceil", "", replaceBraces, true);

  // Replace "@" with "\lceil"
  replaceAll(result, "@", "\\lceil", false);

  // Replace "#" with "\rceil"
  replaceAll(result, "#", "\\rceil", false);

  // Remove "*" (multiplication)
  std::erase(result, '*');

  // Remove "(x,y,z)"
  replaceAll(result, "(x,y,z)", "", false);

  // Replace "{(...)}" with "{...}"
  static RE2 const regexNameInParensInCurlyBraces{R"del(\{\((.+)\)\})del"};
  assert(regexNameInParensInCurlyBraces.ok()); // NOLINT
  encloseMatchesInCurlyBraces(result, regexNameInParensInCurlyBraces);

  m_exprMathJax = result;
}

void removeWhitespaces(std::string &str) {
  std::array const whitespaces{' ', '\f', '\n', '\r', '\t', '\v'};
  for (auto const &whitespace : whitespaces) {
    std::erase(str, whitespace);
  }
}

void parenthesizeFunctionCalls(std::string &str) {
  std::unordered_set<std::string> functionCalls;

  // Match "name(" where name is a function name
  static RE2 const regex(R"del(\b([a-zA-Z_]*\w*\s*\())del");
  assert(regex.ok()); // NOLINT

  re2::StringPiece match;

  for (std::string ns{str}; RE2::PartialMatch(ns, regex, &match);) {
    auto const matchPosition{
        gsl::narrow<std::size_t>(match.data() - ns.c_str())};
    auto const advancePos{matchPosition + match.size()};
    auto const startPos{advancePos - 1};
    auto const bracePos{getBracesPos(ns, startPos, {'(', ')'})};
    auto const callArgs{
        ns.substr(bracePos.first + 1, bracePos.second - bracePos.first)};

    functionCalls.emplace(match.as_string() + callArgs); // "name(" + "...)"

    ns = ns.substr(advancePos);
  }

  for (auto const &fcall : functionCalls) {
    replaceAll(str, fcall, fmt::format("({})", fcall));
  }
}

void reformatStringNumbersAsFloats(std::string &str) {
  // Match integers (e.g. 42) or floats (e.g. .42 or 4.2)
  static RE2 const regex(R"del(((\.\d+\.?\d*)|\b(\d+\.?\d*)))del");
  assert(regex.ok()); // NOLINT

  std::size_t pos{};
  re2::StringPiece match;

  for (std::string ns{str}; RE2::PartialMatch(ns, regex, &match);) {
    auto const matchPosition{
        gsl::narrow<std::size_t>(match.data() - ns.c_str())};

    auto const number{std::stod(match.as_string())};
    auto integralPart{0.0};
    auto const fractionalPart{std::modf(number, &integralPart)};
    auto const formattedString{FP_ZERO == std::fpclassify(fractionalPart)
                                   ? fmt::format("{:.1f}", number)
                                   : fmt::format("{:.12g}", number)};
    str.replace(pos + matchPosition, match.size(), formattedString);
    pos += formattedString.length() + matchPosition;

    ns = ns.substr(matchPosition + match.size());
  }
}

void Equation::convertToGLSL() {
  std::array const variables{"x", "y", "z"};

  std::string result{m_loadedData.expression};
  removeWhitespaces(result);

  // Replace [] with ()
  replaceAll(result, "[", "(");
  replaceAll(result, "]", ")");

  // Replace fun(...) with (fun(...))
  parenthesizeFunctionCalls(result);

  // Replace x^y with mpowy(x) or mpow(x,y)
  auto const maxPowerByMultiplication{16.0};
  for (std::string::size_type idx{}; idx < result.length(); ++idx) {
    if (result[idx] == '^') {
      auto const operandSizes{getOperandSizes(result, idx)};
      auto const exprSize{operandSizes.first + operandSizes.second + 1};
      auto const exprFirstPos{idx - operandSizes.first};
      auto const leftOperand{result.substr(exprFirstPos, operandSizes.first)};
      auto const rightOperand{result.substr(
          exprFirstPos + operandSizes.first + 1, operandSizes.second)};

      char *end{};
      auto power{std::strtod(rightOperand.c_str(), &end)};
      auto isIntegral{false};
      if (power > 0.0 && power <= maxPowerByMultiplication) {
        auto integralPart{0.0};
        auto fractionalPart{std::modf(power, &integralPart)};
        isIntegral = FP_ZERO == std::fpclassify(fractionalPart);
      }
      std::string with{};
      if (isIntegral) {
        with = gsl::narrow_cast<int>(power) == 1
                   ? fmt::format("({})", leftOperand)
                   : fmt::format("mpow{:.0f}({})", power, leftOperand);
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

  // Reformat integrals as floats (e.g. 42 as 42.0)
  reformatStringNumbersAsFloats(result);

  m_exprGLSL = result;
}

[[nodiscard]] std::string Equation::getMathJaxExpression(float isoValue) const {
  // Append "= isoValue"
  return fmt::format("{}={:.3g}", m_exprMathJax, isoValue);
};