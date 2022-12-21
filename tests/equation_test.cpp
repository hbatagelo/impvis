#include <gtest/gtest.h>

#include "equation.cpp" // NOLINT

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,
// cppcoreguidelines-owning-memory)

/**
 * getBracketsPos
 **/

// Test the function with a string that does not contain any enclosing brackets
TEST(GetBracketsPosTest, NoEnclosingBrackets) {
  auto const *const str{"f  *x+1"};
  auto const pos{1};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that does not contain the closing bracket
TEST(GetBracketsPosTest, NoClosingBracket) {
  auto const *const str{"f((x)+"};
  auto const pos{1};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with string that has a non-whitespace character from the
// pos parameter and the first bracket
TEST(GetBracketsPosTest, NonWhitespaceCharBeforeFirstBracket) {
  auto const *const str{"f(x)"};
  auto const pos{0};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets but no nested
// brackets
TEST(GetBracketsPosTest, BracketsFoundWithNoNestedBrackets) {
  auto const *const str{"f(x)"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{1, 3}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets and one level of
// nested brackets
TEST(GetBracketsPosTest, BracketsFoundWithNestedBrackets) {
  auto const *const str{"f   ((x)+1)"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 10}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets and multiple
// levels of nested brackets
TEST(GetBracketsPosTest, BracketsFoundWithMultipleNestedBrackets) {
  auto const *const str{"f   (((x)+1)+1)"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 14}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets of different
// types
TEST(GetBracketsPosTest, DifferentEnclosingBrackets) {
  auto const *const str{"f   {{{x]+1]+1]"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 14}};
  auto const output{getBracketsPos(str, pos, std::pair{'{', ']'})};
  EXPECT_EQ(output, expectedOutput);
}

/**
 * getBracketsPosReverse
 **/

// Test the function with a string that has no enclosing brackets
TEST(GetBracketsPosReverseTest, NoEnclosingBrackets) {
  auto const *const str{"x+1  "};
  auto const pos{4};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that does not contain the opening bracket
TEST(GetBracketsPosReverseTest, NoOpeningBracket) {
  auto const *const str{"(x)+1)"};
  auto const pos{5};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with string that has a non-whitespace character from the
// pos parameter and the first bracket
TEST(GetBracketsPosReverseTest, NonWhitespaceCharBeforeFirstBracket) {
  auto const *const str{"(x)+1"};
  auto const pos{3};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets but no nested
// Brackets
TEST(GetBracketsPosReverseTest, BracketsFoundWithNoNestedBrackets) {
  auto const *const str{"x+(x)"};
  auto const pos{4};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 2}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets and one level of
// nested brackets
TEST(GetBracketsPosReverseTest, BracketsFoundWithNestedBrackets) {
  auto const *const str{"1+((x)+1)  +x"};
  auto const pos{10};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{8, 2}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets and multiple
// levels of nested brackets
TEST(GetBracketsPosReverseTest, BracketsFoundWithMultipleNestedBrackets) {
  auto const *const str{"1+(((x)+1)+1)  +x"};
  auto const pos{14};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{12, 2}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets of different
// types
TEST(GetBracketsPosReverseTest, DifferentEnclosingBrackets) {
  auto const *const str{"1+{{{x]+1]+1]  +x"};
  auto const pos{14};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{12, 2}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'{', ']'})};
  EXPECT_EQ(output, expectedOutput);
}

/**
 * getSizesOfGLSLOperands
 **/

// Test the function with an empty string
TEST(GetSizeOfGLSLOperandsTest, EmptyString) {
  auto const *const str{""};
  auto const pos{0};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{0, 0}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with wrong position of the operator
TEST(GetSizeOfGLSLOperandsTest, WrongOperatorPosition) {
  auto const *const str{"x-2"};
  auto const pos{2};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{0, 0}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a simple expression
TEST(GetSizeOfGLSLOperandsTest, SimpleExpression) {
  auto const *const str{"x*2"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{1, 1}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with negative operands
TEST(GetSizeOfGLSLOperandsTest, NegativeOperands) {
  auto const *const str{"-4.2/-x"};
  auto const pos{4};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{3, 2}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with an expression that has a non-parenthesized function
// call as right operand
TEST(GetSizeOfGLSLOperandsTest, RightOperandWithNonParenthesizeFunctionCall) {
  auto const *const str{"(2*(x+y))*-sin(var2_+(y - 3.14))"};
  auto const pos{9};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{9, 4}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with an expression that has a parenthesized function call
// as right operand
TEST(GetSizeOfGLSLOperandsTest, RightOperandWithParenthesizeFunctionCall) {
  auto const *const str{"(2*(x+y))*-(sin(var2_+(y - 3.14)))"};
  auto const pos{9};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{9, 24}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with variables as operands
TEST(GetSizeOfGLSLOperandsTest, VariableAsOperands) {
  auto const *const str{"1/_myVar123^foo42bar*(x+1)"};
  auto const pos{11};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{9, 8}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with floating point numbers as operands
TEST(GetSizeOfGLSLOperandsTest, ExpressionWithFloatingPointNumbers) {
  auto const *const str{"3.14*2.3"};
  auto const pos{4};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 3}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has no operands
TEST(GetSizeOfGLSLOperandsTest, NoOperands) {
  auto const *const str{"+"};
  auto const pos{0};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{0, 0}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has only a left operand
TEST(GetSizeOfGLSLOperandsTest, OnlyLeftOperand) {
  auto const *const str{"x/"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{1, 0}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has only a right operand
TEST(GetSizeOfGLSLOperandsTest, OnlyRightOperand) {
  auto const *const str{"+x"};
  auto const pos{0};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{0, 1}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,
// cppcoreguidelines-owning-memory)
