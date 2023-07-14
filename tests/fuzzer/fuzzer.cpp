#include "equation.hpp"

namespace {
void parseEquation(uint8_t const *data, std::size_t size) {
  Equation::LoadedData loadedData;

  // NOLINTNEXTLINE(*reinterpret-cast)
  std::string_view stringView{reinterpret_cast<char const *>(data), size};
  loadedData.expression = stringView;

  Equation equation(loadedData);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, std::size_t size) {
  parseEquation(data, size);
  return 0;
}