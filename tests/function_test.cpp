#include <gtest/gtest.h>

#include "function.hpp"
#include "function_test.hpp"

/**
 * getBracketsPos
 **/

// Test the function with a string that does not contain any enclosing brackets
TEST(GetBracketsPosTest, NoEnclosingBrackets) {
  std::string_view str{"f  *x+1"};
  auto const pos{1};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that does not contain the closing bracket
TEST(GetBracketsPosTest, NoClosingBracket) {
  std::string_view str{"f((x)+"};
  auto const pos{1};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with string that has a non-whitespace character from the
// pos parameter and the first bracket
TEST(GetBracketsPosTest, NonWhitespaceCharBeforeFirstBracket) {
  std::string_view str{"f(x)"};
  auto const pos{0};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets but no nested
// brackets
TEST(GetBracketsPosTest, BracketsFoundWithNoNestedBrackets) {
  std::string_view str{"f(x)"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{1, 3}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets and one level of
// nested brackets
TEST(GetBracketsPosTest, BracketsFoundWithNestedBrackets) {
  std::string_view str{"f   ((x)+1)"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 10}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets and multiple
// levels of nested brackets
TEST(GetBracketsPosTest, BracketsFoundWithMultipleNestedBrackets) {
  std::string_view str{"f   (((x)+1)+1)"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 14}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets of different
// types
TEST(GetBracketsPosTest, DifferentEnclosingBrackets) {
  std::string_view str{"f   {{{x]+1]+1]"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 14}};
  auto const output{getBracketsPos(str, pos, std::pair{'{', ']'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test with empty string
TEST(GetBracketsPosTest, EmptyString) {
  std::string_view str;
  auto const pos{0};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test with position at end of string
TEST(GetBracketsPosTest, PositionAtEnd) {
  std::string_view str{"f(x)"};
  auto const pos{4};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPos(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

/**
 * getBracketsPosReverse
 **/

// Test the function with a string that has no enclosing brackets
TEST(GetBracketsPosReverseTest, NoEnclosingBrackets) {
  std::string_view str{"x+1  "};
  auto const pos{4};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that does not contain the opening bracket
TEST(GetBracketsPosReverseTest, NoOpeningBracket) {
  std::string_view str{"(x)+1)"};
  auto const pos{5};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with string that has a non-whitespace character from the
// pos parameter and the first bracket
TEST(GetBracketsPosReverseTest, NonWhitespaceCharBeforeFirstBracket) {
  std::string_view str{"(x)+1"};
  auto const pos{3};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets but no nested
// Brackets
TEST(GetBracketsPosReverseTest, BracketsFoundWithNoNestedBrackets) {
  std::string_view str{"x+(x)"};
  auto const pos{4};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 2}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets and one level of
// nested brackets
TEST(GetBracketsPosReverseTest, BracketsFoundWithNestedBrackets) {
  std::string_view str{"1+((x)+1)  +x"};
  auto const pos{10};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{8, 2}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets and multiple
// levels of nested brackets
TEST(GetBracketsPosReverseTest, BracketsFoundWithMultipleNestedBrackets) {
  std::string_view str{"1+(((x)+1)+1)  +x"};
  auto const pos{14};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{12, 2}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has enclosing brackets of different
// types
TEST(GetBracketsPosReverseTest, DifferentEnclosingBrackets) {
  std::string_view str{"1+{{{x]+1]+1]  +x"};
  auto const pos{14};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{12, 2}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'{', ']'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test with position out of bounds
TEST(GetBracketsPosReverseTest, PositionOutOfBounds) {
  std::string_view str{"(x)"};
  auto const pos{10};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

// Test with position at zero (edge case)
TEST(GetBracketsPosReverseTest, PositionAtZero) {
  std::string_view str{"(x)"};
  auto const pos{0};
  auto const expectedOutput{std::pair{std::string::npos, std::string::npos}};
  auto const output{getBracketsPosReverse(str, pos, std::pair{'(', ')'})};
  EXPECT_EQ(output, expectedOutput);
}

/**
 * getSizesOfGLSLOperands
 **/

// Test the function with an empty string
TEST(GetSizeOfGLSLOperandsTest, EmptyString) {
  std::string_view str;
  auto const pos{0};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{0, 0}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with wrong position of the operator
TEST(GetSizeOfGLSLOperandsTest, WrongOperatorPosition) {
  std::string_view str{"x-2"};
  auto const pos{2};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{0, 0}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a simple expression
TEST(GetSizeOfGLSLOperandsTest, SimpleExpression) {
  std::string_view str{"x*2"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{1, 1}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with negative operands
TEST(GetSizeOfGLSLOperandsTest, NegativeOperands) {
  std::string_view str{"-4.2/-x"};
  auto const pos{4};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{3, 2}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with an expression that has a non-parenthesized function
// call as right operand
TEST(GetSizeOfGLSLOperandsTest, RightOperandWithNonParenthesizeFunctionCall) {
  std::string_view str{"(2*(x+y))*-sin(var2_+(y - 3.14))"};
  auto const pos{9};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{9, 4}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with an expression that has a parenthesized function call
// as right operand
TEST(GetSizeOfGLSLOperandsTest, RightOperandWithParenthesizeFunctionCall) {
  std::string_view str{"(2*(x+y))*-(sin(var2_+(y - 3.14)))"};
  auto const pos{9};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{9, 24}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with variables as operands
TEST(GetSizeOfGLSLOperandsTest, VariableAsOperands) {
  std::string_view str{"1/_myVar123^foo42bar*(x+1)"};
  auto const pos{11};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{9, 8}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with floating point numbers as operands
TEST(GetSizeOfGLSLOperandsTest, ExpressionWithFloatingPointNumbers) {
  std::string_view str{"3.14*2.3"};
  auto const pos{4};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{4, 3}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has no operands
TEST(GetSizeOfGLSLOperandsTest, NoOperands) {
  std::string_view str{"+"};
  auto const pos{0};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{0, 0}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has only a left operand
TEST(GetSizeOfGLSLOperandsTest, OnlyLeftOperand) {
  std::string_view str{"x/"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{1, 0}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test the function with a string that has only a right operand
TEST(GetSizeOfGLSLOperandsTest, OnlyRightOperand) {
  std::string_view str{"+x"};
  auto const pos{0};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{0, 1}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test with parenthesized left operand
TEST(GetSizeOfGLSLOperandsTest, ParenthesizedLeftOperand) {
  std::string_view str{"(x+y)*z"};
  auto const pos{5};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{5, 1}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

// Test with parenthesized right operand
TEST(GetSizeOfGLSLOperandsTest, ParenthesizedRightOperand) {
  std::string_view str{"x*(y+z)"};
  auto const pos{1};
  auto const expectedOutput{std::pair<std::size_t, std::size_t>{1, 5}};
  auto const output{getSizesOfGLSLOperands(str, pos)};
  EXPECT_EQ(output, expectedOutput);
}

/**
 * Function class tests
 **/

// Test Function constructor with simple expression
TEST(FunctionTest, ConstructorSimpleExpression) {
  Function::Data data;
  data.expression = "x + y + z";
  Function func(data);

  EXPECT_FALSE(func.getGLSLExpression().empty());
}

// Test Function constructor with power expression
TEST(FunctionTest, ConstructorPowerExpression) {
  Function::Data data;
  data.expression = "x^2";
  Function func(data);

  auto const &glsl{func.getGLSLExpression()};
  // Should convert x^2 to mpow2(x)
  EXPECT_NE(glsl.find("mpow2"), std::string::npos);
}

// Test Function constructor with division
TEST(FunctionTest, ConstructorDivisionExpression) {
  Function::Data data;
  data.expression = "x/y";
  Function func(data);

  EXPECT_FALSE(func.getGLSLExpression().empty());
}

// Test getMathJaxEquation
TEST(FunctionTest, GetMathJaxEquation) {
  Function::Data data;
  data.expression = "x^2 + y^2";
  Function func(data);

  auto const equation{func.getMathJaxEquation(1.0f)};
  EXPECT_FALSE(equation.empty());
  EXPECT_NE(equation.find("x^{2}+y^{2}=1"), std::string::npos);
}

// Test parameter extraction with simple variable
TEST(FunctionTest, ParameterExtractionSimpleVariable) {
  Function::Data data;
  data.expression = "a*x + b*y";
  Function func(data);

  auto const &params = func.getParameters();
  EXPECT_EQ(params.size(), 2);

  // Check that a and b are in parameters
  auto hasA{false};
  auto hasB{false};
  for (auto const &p : params) {
    if (p.name == "a") {
      hasA = true;
    }
    if (p.name == "b") {
      hasB = true;
    }
  }
  EXPECT_TRUE(hasA);
  EXPECT_TRUE(hasB);
}

// Test parameter extraction with underscores in identifiers
TEST(FunctionTest, ParameterExtractionWithUnderscores) {
  Function::Data data;
  data.expression = "my_param*x + another_var*y";
  Function func(data);

  auto const &params = func.getParameters();
  EXPECT_EQ(params.size(), 2);

  // Check that my_param and another_var are extracted as complete identifiers
  auto hasMyParam{false};
  auto hasAnotherVar{false};
  for (auto const &p : params) {
    if (p.name == "my_param") {
      hasMyParam = true;
    }
    if (p.name == "another_var") {
      hasAnotherVar = true;
    }
  }
  EXPECT_TRUE(hasMyParam);
  EXPECT_TRUE(hasAnotherVar);
}

// Test that reserved names are not extracted as parameters
TEST(FunctionTest, ParameterExtractionReservedNames) {
  Function::Data data;
  data.expression = "x + y + z + pi";
  Function func(data);

  auto const &params{func.getParameters()};
  // x, y, z, pi are all reserved, so no parameters
  EXPECT_EQ(params.size(), 0);
}

// Test setParameter
TEST(FunctionTest, SetParameter) {
  Function::Data data;
  data.expression = "a*x";
  Function func(data);

  EXPECT_TRUE(func.setParameter("a", 5.0f));

  auto const &params{func.getParameters()};
  EXPECT_EQ(params.size(), 1);
  EXPECT_EQ(params[0].value, 5.0f);
}

// Test setParameter with non-existent parameter
TEST(FunctionTest, SetNonExistentParameter) {
  Function::Data data;
  data.expression = "x + y";
  Function func(data);

  EXPECT_FALSE(func.setParameter("nonexistent", 1.0f));
}

// Test conversion with brackets
TEST(FunctionTest, ConversionWithBrackets) {
  Function::Data data;
  data.expression = "[x + y]";
  Function func(data);

  // Brackets should be converted to parentheses in GLSL
  auto const &glsl{func.getGLSLExpression()};
  EXPECT_EQ(glsl.find('['), std::string::npos);
  EXPECT_EQ(glsl.find(']'), std::string::npos);
}

// Test conversion with function calls
TEST(FunctionTest, ConversionWithFunctionCalls) {
  Function::Data data;
  data.expression = "sin(x) + cos(y)";
  Function func(data);

  auto const &glsl{func.getGLSLExpression()};
  EXPECT_FALSE(glsl.empty());
}

// Test conversion with exponentiation using **
TEST(FunctionTest, ConversionWithDoubleStarExponentiation) {
  Function::Data data;
  data.expression = "x**3";
  Function func(data);

  auto const &glsl{func.getGLSLExpression()};
  // Should be converted to mpow
  EXPECT_NE(glsl.find("mpow"), std::string::npos);
}

// Test MathJax conversion with Greek letters
TEST(FunctionTest, MathJaxWithGreekLetters) {
  Function::Data data;
  data.expression = "alpha*x + beta*y";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  // Should contain \alpha and \beta
  EXPECT_NE(equation.find("\\alpha"), std::string::npos);
  EXPECT_NE(equation.find("\\beta"), std::string::npos);
}

// Test MathJax conversion with sqrt
TEST(FunctionTest, MathJaxWithSqrt) {
  Function::Data data;
  data.expression = "sqrt(x)";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  EXPECT_NE(equation.find("\\sqrt"), std::string::npos);
}

// Test MathJax conversion with abs
TEST(FunctionTest, MathJaxWithAbs) {
  Function::Data data;
  data.expression = "abs(x)";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  // abs should be converted to |x|
  EXPECT_NE(equation.find('|'), std::string::npos);
}

// Test MathJax conversion with floor
TEST(FunctionTest, MathJaxWithFloor) {
  Function::Data data;
  data.expression = "floor(x)";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  EXPECT_NE(equation.find("\\lfloor"), std::string::npos);
  EXPECT_NE(equation.find("\\rfloor"), std::string::npos);
}

// Test MathJax conversion with ceil
TEST(FunctionTest, MathJaxWithCeil) {
  Function::Data data;
  data.expression = "ceil(x)";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  EXPECT_NE(equation.find("\\lceil"), std::string::npos);
  EXPECT_NE(equation.find("\\rceil"), std::string::npos);
}

// Test MathJax conversion with exp
TEST(FunctionTest, MathJaxWithExp) {
  Function::Data data;
  data.expression = "exp(x)";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  // exp(x) should become e^{x}
  EXPECT_NE(equation.find("e^"), std::string::npos);
}

// Test MathJax conversion with log
TEST(FunctionTest, MathJaxWithLog) {
  Function::Data data;
  data.expression = "log(x)";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  EXPECT_NE(equation.find("\\ln"), std::string::npos);
}

// Test Function with newline in expression
TEST(FunctionTest, ExpressionWithNewline) {
  Function::Data data;
  data.expression = "x +\\ny";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  // Should handle newlines
  EXPECT_FALSE(equation.empty());
}

// Test getData returns correct data
TEST(FunctionTest, GetData) {
  Function::Data data;
  data.name = "Test Function";
  data.expression = "x + y";
  data.boundsRadius = 3.5f;
  Function func(data);

  auto const &returnedData{func.getData()};
  EXPECT_EQ(returnedData.name, "Test Function");
  EXPECT_EQ(returnedData.expression, "x + y");
  EXPECT_FLOAT_EQ(returnedData.boundsRadius, 3.5f);
}

// Test with complex nested expression
TEST(FunctionTest, ComplexNestedExpression) {
  Function::Data data;
  data.expression = "((x + y) * (z - 1)) / (a + b)";
  Function func(data);

  EXPECT_FALSE(func.getGLSLExpression().empty());
  auto const &params{func.getParameters()};
  EXPECT_EQ(params.size(), 2); // a and b
}

// Test parameter values from loaded data
TEST(FunctionTest, ParameterValuesFromLoadedData) {
  Function::Data data;
  data.expression = "a*x + b*y";
  data.parameters = {{"a", 2.5f}, {"b", 3.5f}};
  Function func(data);

  auto const &params{func.getParameters()};
  EXPECT_EQ(params.size(), 2);

  for (auto const &p : params) {
    if (p.name == "a") {
      EXPECT_FLOAT_EQ(p.value, 2.5f);
    } else if (p.name == "b") {
      EXPECT_FLOAT_EQ(p.value, 3.5f);
    }
  }
}

// Test integer conversion to float in GLSL
TEST(FunctionTest, IntegerToFloatConversion) {
  Function::Data data;
  data.expression = "x + 42";
  Function func(data);

  auto const &glsl{func.getGLSLExpression()};
  // 42 should be converted to 42.0
  EXPECT_NE(glsl.find("42.0"), std::string::npos);
}

// Test with fractional number
TEST(FunctionTest, FractionalNumberConversion) {
  Function::Data data;
  data.expression = "x + 3.14";
  Function func(data);

  auto const &glsl{func.getGLSLExpression()};
  EXPECT_NE(glsl.find("3.14"), std::string::npos);
}

// Test MathJax with fractions
TEST(FunctionTest, MathJaxWithFractions) {
  Function::Data data;
  data.expression = "x/y";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  // Division should become \frac
  EXPECT_NE(equation.find("\\frac"), std::string::npos);
}

// Test MathJax with complex fractions
TEST(FunctionTest, MathJaxWithComplexFractions) {
  Function::Data data;
  data.expression = "(x+1)/(y+2)";
  Function func(data);

  auto const equation{func.getMathJaxEquation(0.0f)};
  EXPECT_NE(equation.find("\\frac"), std::string::npos);
}

// Test power with integer exponent in valid range
TEST(FunctionTest, PowerWithIntegerExponent) {
  Function::Data data;
  data.expression = "x^5";
  Function func(data);

  auto const &glsl{func.getGLSLExpression()};
  // Should use mpow5 for integer exponent
  EXPECT_NE(glsl.find("mpow5"), std::string::npos);
}

// Test power with exponent = 1
TEST(FunctionTest, PowerWithExponentOne) {
  Function::Data data;
  data.expression = "x^1";
  Function func(data);

  auto const &glsl{func.getGLSLExpression()};
  // x^1 should just become (x)
  EXPECT_EQ(glsl.find("mpow"), std::string::npos);
}

// Test power with fractional exponent
TEST(FunctionTest, PowerWithFractionalExponent) {
  Function::Data data;
  data.expression = "x^2.5";
  Function func(data);

  auto const &glsl{func.getGLSLExpression()};
  // Should use mpow(x, 2.5) for fractional exponent
  EXPECT_NE(glsl.find("mpow"), std::string::npos);
}
