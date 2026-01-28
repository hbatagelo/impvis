/**
 * @file util.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <abcgOpenGLExternal.hpp>

#include <gsl/gsl>

namespace util {

template <typename Fun>
concept ReplaceCallback =
    std::invocable<Fun, std::string &, std::string::size_type>;

// Replace all occurrences of the string 'what' in 'inout' with the string
// 'with'. For each replacement, 'replaceCallback' is called with arguments
// (s, n), where 's' is a reference to 'inout' and 'n' is the position of the
// character immediately after the last replacement.
std::size_t replaceAllAndInvoke(std::string &inout, std::string_view what,
                                std::string_view with,
                                ReplaceCallback auto &&replaceCallback,
                                bool matchIdentifier = false) {
  if (what.empty()) {
    return 0;
  }

  auto const isIdentifier{[](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
  }};

  std::size_t count{};
  for (auto pos{inout.find(what)}; pos != std::string::npos;
       pos = inout.find(what, pos)) {

    if (matchIdentifier) {
      auto const posFirstChar{pos};
      auto const posLastChar{pos + what.length() - 1};
      auto const wholeWord{
          (posFirstChar == 0 || !isIdentifier(inout.at(posFirstChar - 1))) &&
          (posLastChar >= inout.size() - 1 ||
           !isIdentifier(inout.at(posLastChar + 1)))};
      if (!wholeWord) {
        ++pos;
        continue;
      }
    }

    inout.replace(pos, what.length(), with.data(), with.length());

    pos += with.length();

    replaceCallback(inout, pos);

    ++count;
  }

  return count;
}

// Replace all occurrences of the string 'what' in 'inout' with the string
// 'with'.
inline std::size_t replaceAll(std::string &inout, std::string_view what,
                              std::string_view with,
                              bool matchIdentifier = false) {
  return replaceAllAndInvoke(
      inout, what, with, [](auto &, auto) {}, matchIdentifier);
}

// Converts a string_view to a lowercase std::string (ASCII only).
inline std::string toLower(std::string_view str) {
  std::string result{str};
  std::ranges::transform(result, result.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return result;
}

} // namespace util

#endif