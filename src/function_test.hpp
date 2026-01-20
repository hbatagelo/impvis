/**
 * @file function_test.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef FUNCTION_TEST_HPP_
#define FUNCTION_TEST_HPP_

#include <string>
#include <string_view>
#include <utility>

// Functions exposed for testing
std::pair<std::string::size_type, std::string::size_type>
getBracketsPos(std::string_view str, std::string::size_type pos,
               std::pair<char, char> brackets);

std::pair<std::string::size_type, std::string::size_type>
getBracketsPosReverse(std::string_view str, std::string::size_type pos,
                      std::pair<char, char> brackets);

std::pair<std::size_t, std::size_t> getSizesOfGLSLOperands(std::string_view str,
                                                           std::size_t pos);

#endif // FUNCTION_TEST_HPP_