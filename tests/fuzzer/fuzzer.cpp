#include "equation.hpp"

static void parseEquation(uint8_t const *data, std::size_t size) {
  Equation::LoadedData loadedData;

  std::string_view stringView{reinterpret_cast<char const *>(data), // NOLINT
                              size};
  loadedData.expression = stringView;

  Equation equation(loadedData);
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, std::size_t size) {
  parseEquation(data, size);
  return 0;
}