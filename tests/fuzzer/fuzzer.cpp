#include "function.hpp"

namespace {
void parseFunction(uint8_t const *input, std::size_t size) {
  // Cap size to prevent pathological allocations
  constexpr std::size_t maxSize{1 << 20};
  size = std::min(size, maxSize);

  // NOLINTNEXTLINE(*reinterpret-cast)
  std::string_view stringView{reinterpret_cast<char const *>(input), size};

  Function::Data data;
  data.expression = stringView;

  try {
    Function function(data);
  } catch (...) {
    // Swallow exceptions
  }
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, std::size_t size) {
  parseFunction(data, size);
  return 0;
}