/**
 * @file util.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022 Harlen Batagelo. All rights reserved.
 * This project is released under the MIT license.
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <string>

// Syntax requirement of the callback function used by replaceAllAndInvoke
template <typename Fun>
concept ReplaceCallback =
    std::is_invocable_v<Fun, std::string &, std::string::size_type>;

// Replace all occurrences of the string 'what' in 'inout' with the string
// 'with'. For each replacement, 'replaceCallback' is called with arguments
// (s, n), where 's' is a reference to 'inout' and 'n' is the position of the
// character immediately after the last replacement.
std::size_t replaceAllAndInvoke(std::string &inout, std::string_view what,
                                std::string_view with,
                                ReplaceCallback auto &&replaceCallback,
                                bool matchWholeWord = false) {
  std::size_t count{};
  for (std::string::size_type pos{};
       std::string::npos !=
       (pos = inout.find(what.data(), pos, what.length()));) {

    if (matchWholeWord) {
      auto const posFirstChar{pos};
      auto const posLastChar{pos + what.length() - 1};
      auto const wholeWord{
          (posFirstChar == 0 || !std::isalpha(inout.at(posFirstChar - 1))) &&
          (posLastChar >= inout.size() - 1 ||
           !std::isalpha(inout.at(posLastChar + 1)))};
      if (!wholeWord) {
        ++pos;
        continue;
      }
    }

    inout.replace(pos, what.length(), with.data(), with.length());

    replaceCallback(inout, pos + with.length());

    pos += with.length();
    ++count;
  }
  return count;
}

// Replace all occurrences of the string 'what' in 'inout' with the string
// 'with'.
inline std::size_t replaceAll(std::string &inout, std::string_view what,
                              std::string_view with,
                              bool matchWholeWord = false) {
  auto dummyLambda{[](std::string &, std::string::size_type) {}};
  return replaceAllAndInvoke(inout, what, with, dummyLambda, matchWholeWord);
}

#endif