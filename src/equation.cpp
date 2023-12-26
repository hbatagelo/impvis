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
#include <string_view>

#include "equation.hpp"
#include "util.hpp"

Equation::Equation(LoadedData data) : m_loadedData(std::move(data)) {
  replaceAll(m_loadedData.expression, "\\n", "\n");
  convertToGLSL();
  convertToMathJax();
}

void Equation::onCreate() {
  auto const path{abcg::Application::getAssetsPath()};
  if (!m_loadedData.thumbnail.empty()) {
    m_thumbnailId =
        abcg::loadOpenGLTexture({.path = path + m_loadedData.thumbnail,
                                 .generateMipmaps = false,
                                 .flipUpsideDown = false});
  }
}

void Equation::onDestroy() { abcg::glDeleteTextures(1, &m_thumbnailId); }

[[nodiscard]] std::string Equation::getMathJaxExpression(float isoValue) const {
  return fmt::format("{}={:.3g}", m_exprMathJax, isoValue);
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

// Returns the position of the first pair of enclosing brackets starting from
// str[pos].
//
// Returns (npos, npos) if the brackets are not found or if there is a
// non-whitespace character from pos and the opening bracket.
//
// Examples using parens:
// str="f(x)"        pos=1 -> returns (1,3)
//       ^
// str="f   ((x)+1)" pos=1 -> returns (4,10)
//       ^
// str="f(x)"        pos=0 -> returns (npos, npos)
//      ^
std::pair<std::string::size_type, std::string::size_type>
getBracketsPos(std::string_view str, std::string::size_type pos,
               std::pair<char, char> brackets) {
  auto const failed{std::pair{std::string::npos, std::string::npos}};

  // Advance through whitespaces, from pos to the position of the opening
  // bracket. If any non-whitespace character is found, return (npos, npos)
  auto startPos{pos};
  for (; startPos < str.length(); ++startPos) {
    auto const chr{str.at(startPos)};
    if (chr == brackets.first) {
      break; // Opening bracket found
    }
    if (std::isspace(chr) == 0) {
      return failed;
    }
  }

  // Look for corresponding closing bracket
  if (!str.empty()) {
    auto numNestedParens{0};
    for (auto endPos{startPos}; endPos < str.length() - 1; ++endPos) {
      auto const chr{str.at(endPos + 1)};
      if (chr == brackets.first) {
        ++numNestedParens;
      }
      if (chr == brackets.second) {
        if (numNestedParens > 0) {
          --numNestedParens;
        } else {
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
// brackets starting from str[pos].
//
// Returns (npos, npos) if the brackets are not found or if there is a
// non-whitespace character from pos and the opening bracket.
//
// Examples using parens:
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

  // Reverse through whitespaces, from pos to the position of the closing
  // bracket. If any non-whitespace character is found, return (npos, npos)
  auto startPos{pos};
  for (; startPos > 0; --startPos) {
    auto const chr{str.at(startPos)};
    if (chr == brackets.second) {
      break; // Closing bracket found
    }
    if (std::isspace(chr) == 0) {
      return failed;
    }
  }
  if (startPos == 0) {
    return failed;
  }

  // Look for corresponding opening bracket
  auto numNestedParens{0};
  for (auto endPos{startPos}; endPos > 0; --endPos) {
    auto const chr{str.at(endPos - 1)};
    if (chr == brackets.second) {
      ++numNestedParens;
    }
    if (chr == brackets.first) {
      if (numNestedParens > 0) {
        --numNestedParens;
      } else {
        --endPos;
        return std::pair{startPos, endPos};
      }
    }
  }
  return failed;
}

void encloseFunctionCallsInBrackets(std::string &str,
                                    std::pair<char, char> brackets) {
  std::unordered_set<std::string> functionCalls;

  // Match "name(" where name is a function name
  static RE2 const regex(R"re(\b([a-zA-Z_]*\w*\s*\())re");
  assert(regex.ok()); // NOLINT

  re2::StringPiece match;

  for (std::string ns{str}; RE2::PartialMatch(ns, regex, &match);) {
    auto const matchPosition{
        gsl::narrow<std::size_t>(match.data() - ns.c_str())};
    auto const advancePos{matchPosition + match.size()};
    auto const startPos{advancePos - 1};
    auto const bracketPos{getBracketsPos(ns, startPos, {'(', ')'})};
    auto const callArgs{
        ns.substr(bracketPos.first + 1, bracketPos.second - bracketPos.first)};

    functionCalls.emplace(match.as_string() + callArgs); // "name(" + "...)"

    ns = ns.substr(advancePos);
  }

  for (auto const &fcall : functionCalls) {
    replaceAll(str, fcall,
               fmt::format("{}{}{}", brackets.first, fcall, brackets.second));
  }
}

void encloseMatchesInBrackets(std::string &str, RE2 const &regex,
                              std::pair<char, char> brackets) {
  std::size_t pos{};
  re2::StringPiece match;

  for (std::string ns{str}; RE2::PartialMatch(ns, regex, &match);) {
    auto const matchPosition{
        gsl::narrow<std::size_t>(match.data() - ns.c_str())};
    auto const posAfterName{matchPosition + match.size()};
    auto const bracketPos{getBracketsPos(ns, posAfterName, {'(', ')'})};

    std::string bracketedArgs{};
    if (bracketPos.first != std::string::npos &&
        bracketPos.second != std::string::npos) {
      assert(bracketPos.second >= bracketPos.first); // NOLINT
      bracketedArgs =
          ns.substr(bracketPos.first, 1 + bracketPos.second - bracketPos.first);
    }

    // "name" + "(...)"
    auto const what{match.as_string() + bracketedArgs};
    // "{name(...)}" if brackets are curly brackets
    auto const with{brackets.first + what + brackets.second};
    str.replace(pos + matchPosition, what.length(), with.data(), with.length());
    ns.replace(matchPosition, what.length(), with.data(), with.length());

    auto const advancePos{matchPosition + match.size()};
    pos += advancePos;
    ns = ns.substr(advancePos);
  }
}

// Returns the sizes of the left and right operands of the single-character
// operator at str[pos], where str in a GLSL expression with no whitespaces. If
// an operand is a function call, the size of the operand will be different
// depending on whether the operand is enclosed within parentheses. For example:
// - In "a+sin(x)", the size of right operand is 3 ("sin").
// - In "a+(sin(x))", the size of right operand is 8 ("(sin(x))").
std::pair<std::size_t, std::size_t>
getSizesOfGLSLOperands(std::string const &str, std::size_t pos) {
  auto sizes{std::pair{std::size_t{}, std::size_t{}}};

  if (pos >= str.size()) {
    return sizes;
  }

  auto notNameAndNotNumber{[&](char chr) {
    return std::isalnum(chr) == 0 && chr != '_' && chr != '.';
  }};

  // Compute size of right operand
  if (auto itr{std::next(str.begin(), gsl::narrow<std::ptrdiff_t>(pos + 1))};
      itr != str.end()) {

    if (*itr == '-' || *itr == '+') {
      itr = std::next(itr);
    }

    if (*itr == '(') {
      // Opening bracket
      auto const startPos{std::distance(str.begin(), itr)};
      auto const bracketPos{
          getBracketsPos(str, gsl::narrow<std::size_t>(startPos), {'(', ')'})};
      if (bracketPos.first != std::string_view::npos) {
        sizes.second = bracketPos.second - pos;
      }
    } else {
      itr = std::find_if(itr, str.end(), notNameAndNotNumber);
      auto const endPos{std::distance(str.begin(), itr)};
      auto const opSize{endPos - gsl::narrow<std::ptrdiff_t>(pos) - 1};
      sizes.second = gsl::narrow<std::size_t>(opSize);
    }
  }

  // Compute size of left operand
  if (auto itr{std::next(str.rbegin(),
                         gsl::narrow<std::ptrdiff_t>(str.size() - pos))};
      itr != str.rend()) {
    if (*itr == ')') {
      // Closing bracket
      auto const startPos{std::distance(itr, str.rend()) - 1};
      auto const bracketPos{getBracketsPosReverse(
          str, gsl::narrow<std::size_t>(startPos), {'(', ')'})};
      if (bracketPos.first != std::string_view::npos) {
        sizes.first = pos - bracketPos.second;
      }
    } else {
      itr = std::find_if(itr, str.rend(), notNameAndNotNumber);
      auto const endPos{std::distance(itr, str.rend()) - 1};
      auto const opSize{gsl::narrow<std::ptrdiff_t>(pos) - endPos - 1};
      sizes.first = gsl::narrow<std::size_t>(opSize);
    }
  }

  return sizes;
}

// Add matches to the given set
void addMatchesToSet(RE2 const &regex, re2::StringPiece str,
                     std::set<std::string> &set) {
  for (std::string match; RE2::FindAndConsume(&str, regex, &match);) {
    set.insert(match);
  }
}

// Remove matches from the given set
void removeMatchesFromSet(RE2 const &regex, re2::StringPiece str,
                          std::set<std::string> &set) {
  for (std::string match; RE2::FindAndConsume(&str, regex, &match);) {
    set.erase(match);
  }
}

// Remove from the given set the matches that are not enclosed by a block scope
void removeMatchesInSameScope(RE2 const &regex, re2::StringPiece str,
                              std::set<std::string> &set) {
  // Helper lambda that checks if the character at `str[pos]` is enclosed
  // by curly brackets (assuming there is an even number of brackets)
  auto insideCurlyBrackets{[](std::string_view view, std::size_t pos) {
    int groups{};
    for (auto const idx : iter::range(pos, view.length())) {
      auto const chr{view.at(idx)};
      if (chr == '{') {
        ++groups;
      }
      if (chr == '}') {
        --groups;
      }
    }
    return groups != 0;
  }};

  for (re2::StringPiece match; RE2::PartialMatch(str, regex, &match);) {
    auto const matchPosition{
        gsl::narrow<std::size_t>(match.data() - str.begin())};
    if (!insideCurlyBrackets(str, matchPosition)) {
      set.erase(match.as_string());
    }
    str.remove_prefix(matchPosition + match.size());
  }
}

void Equation::extractParameters() {
  std::set<std::string> parameters;

  // Add names
  static RE2 const regexName{R"re(([A-Za-z_]\w*))re"};
  assert(regexName.ok()); // NOLINT
  addMatchesToSet(regexName, m_loadedData.expression, parameters);

  // Remove reserved names
  for (auto const &name :
       {"x", "y", "z", "pi", "kBoundRadius", "kBoundRadiusSquared",
        "fragPosition", "outColor", "uCamera", "uShading", "uTransform",
        "uParams", "uIsoValue"}) {
    parameters.erase(name);
  }

  // Remove function names
  static RE2 const regexFunctionName{R"re((([a-zA-Z_])\w*)\s*\()re"};
  assert(regexFunctionName.ok()); // NOLINT
  removeMatchesFromSet(regexFunctionName, m_loadedData.codeGlobal, parameters);
  removeMatchesFromSet(regexFunctionName, m_loadedData.expression, parameters);

  // Remove global constant variables
  static RE2 const regexConstantVariable{
      R"re(const\s+[A-Za-z_]\w*\s+([A-Za-z_]\w*)\s*=)re"};
  assert(regexConstantVariable.ok()); // NOLINT
  removeMatchesInSameScope(regexConstantVariable, m_loadedData.codeGlobal,
                           parameters);

  // Remove local variables
  static RE2 const regexVariable{R"re(\b[A-Za-z_]\w*\s+([A-Za-z_]\w*)\s*=)re"};
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

void reformatStringNumbersAsFloats(std::string &str) {
  // Match integers (e.g. 42) or floats (e.g. .42 or 4.2)
  static RE2 const regex(R"re(((\.\d+\.?\d*)|\b(\d+\.?\d*)))re");
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
  std::string result{m_loadedData.expression};

  // Remove whitespace
  for (auto const &whitespace : {' ', '\f', '\n', '\r', '\t', '\v'}) {
    std::erase(result, whitespace);
  }

  // Replace [] with ()
  replaceAll(result, "[", "(");
  replaceAll(result, "]", ")");

  // Replace fun(...) with (fun(...))
  encloseFunctionCallsInBrackets(result, {'(', ')'});

  // Given the operands of the expression x^y, returns either the string
  // "mpowy(x)" or "mpow(x,y)". The former is returned iff y is an integer in
  // the range [1,16].
  auto mpowExpression{[](std::string_view leftOperand,
                         std::string_view rightOperand) -> std::string {
    char *strEnd{};
    auto const exponent{std::strtod(rightOperand.data(), &strEnd)};
    auto const maxPowerByMultiplication{16.0};
    if (exponent != HUGE_VAL && exponent > 0.0 &&
        exponent <= maxPowerByMultiplication) {
      auto integralPart{0.0};
      auto const fractionalPart{std::modf(exponent, &integralPart)};
      if (auto const isIntegral{FP_ZERO == std::fpclassify(fractionalPart)};
          isIntegral) {
        return gsl::narrow_cast<int>(exponent) == 1
                   ? fmt::format("({})", leftOperand)
                   : fmt::format("mpow{:.0f}({})", exponent, leftOperand);
      }
    }
    return fmt::format("mpow({},{})", leftOperand, rightOperand);
  }};

  // Change all occurrences of x^y with mpowy(x) or mpow(x,y)
  std::size_t idx{};
  while ((idx = result.find_first_of('^', idx)) != std::string::npos) {
    auto const operandSizes{getSizesOfGLSLOperands(result, idx)};
    auto const exprSize{operandSizes.first + operandSizes.second + 1};
    auto const exprStartPosInResult{idx - operandSizes.first};
    auto const leftOperand{
        result.substr(exprStartPosInResult, operandSizes.first)};
    auto const rightOperand{result.substr(
        exprStartPosInResult + operandSizes.first + 1, operandSizes.second)};

    result.replace(exprStartPosInResult, exprSize,
                   mpowExpression(leftOperand, rightOperand));

    idx = exprStartPosInResult;
  }

  // Replace x with @P.@x, y with @P.@y, z with @P.@z. This is necessary to
  // avoid mixing up the user-defined parameter 'p' with the local scope shader
  // variable 'p'
  for (auto const *chr : {"x", "y", "z"}) {
    replaceAll(result, chr, fmt::format("@P.@{}", chr), true);
  }

  // Reformat integrals as floats (e.g. 42 as 42.0)
  reformatStringNumbersAsFloats(result);

  m_exprGLSL = result;
}

void Equation::convertToMathJax() {
  std::string result{m_loadedData.expression};

  std::array const greekLetters{
      "alpha", "beta",  "gamma", "Delta",   "delta",   "epsilon", "zeta",
      "eta",   "Theta", "theta", "iota",    "kappa",   "Lambda",  "lambda",
      "mu",    "nu",    "Xi",    "xi",      "Pi",      "pi",      "rho",
      "Sigma", "sigma", "tau",   "Upsilon", "upsilon", "Phi",     "phi",
      "chi",   "Psi",   "psi",   "Omega",   "omega"};

  // Single-token arguments (variables, parameters and constants)
  std::vector<std::string> singleTokenArgs{"0", "1", "2", "3", "4", "5", "6",
                                           "7", "8", "9", "x", "y", "z"};
  if (m_parameters.empty()) {
    extractParameters();
  }
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
  for (auto const &whitespace : {' ', '\f', '\r', '\t', '\v'}) {
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
  auto replaceBrackets{[&srcBrackets, &dstBrackets](
                           std::string &inout, std::string::size_type pos) {
    if (auto const bracketsPos{getBracketsPos(inout, pos, srcBrackets)};
        bracketsPos.first != std::string::npos) {
      inout.at(bracketsPos.first) = dstBrackets.first;
      inout.at(bracketsPos.second) = dstBrackets.second;
    }
  }};

  // Replace "^expr" with "^{expr}"
  static RE2 const regexExponent{R"re(\^([a-zA-Z_]*[a-zA-Z0-9_.]*\s*))re"};
  assert(regexExponent.ok()); // NOLINT
  encloseMatchesInBrackets(result, regexExponent, {'{', '}'});

  // Replace "name(x)" with "name{x}" where x is a single-token argument
  auto reformatCallWithSingleToken{
      [&singleTokenArgs](std::string &str, std::string_view name) {
        for (auto const &variable : singleTokenArgs) {
          replaceAll(str, fmt::format("{}({})", name, variable),
                     fmt::format("{}{{{}}}", name, variable));
        }
      }};

  for (auto const &name :
       {"asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh",
        "atanh", "sin", "cos", "tan", "min", "max"}) {
    auto const with{std::string{"\\"} + name};
    // Replace "name" with "\name"
    replaceAll(result, name, with, true);
    // f(x) to f{x}
    reformatCallWithSingleToken(result, with);
  }

  // Replace "exp(...)" with "e^{...}"
  replaceAllAndInvoke(result, "exp", "e^", replaceBrackets, true);

  // Replace "exp2(...)" with "2^{...}"
  replaceAllAndInvoke(result, "exp2", "2^", replaceBrackets, true);

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
  static RE2 const regexNameInParensInCurlyBrackets{R"re(\{\((.+)\)\})re"};
  assert(regexNameInParensInCurlyBrackets.ok()); // NOLINT
  encloseMatchesInBrackets(result, regexNameInParensInCurlyBrackets,
                           {'{', '}'});

  m_exprMathJax = result;
}