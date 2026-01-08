#!/bin/bash
set -euo pipefail

BUILD_TYPE=Release

# Reset build directory
rm -rf build
mkdir -p build && cd build

# Create a WASM profile
cat > wasm_profile << 'EOF'
[settings]
arch=wasm
build_type=Release
compiler=clang
compiler.cppstd=20
compiler.libcxx=libc++
compiler.version=21
os=Emscripten

[buildenv]
CC=emcc
CXX=em++
AR=emar
RANLIB=emranlib

[options]
*:opengl=False
*:shared=False
*:fPIC=False

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
tools.build:compiler_executables={"c": "emcc", "cpp": "em++"}
tools.meson.mesontoolchain:backend=ninja
EOF

# Install dependencies with Conan
if ! conan install .. \
  --profile:build=default \
  --profile:host=wasm_profile \
  -b missing \
  -s build_type=$BUILD_TYPE; then
  echo "Error: Conan install failed"
  exit 1
fi

# Wrapper toolchain that chains both toolchains
cat > emscripten_conan_toolchain.cmake << 'EOF'
# Load Emscripten toolchain first
include("$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")

# Then load Conan toolchain
include("${CMAKE_CURRENT_LIST_DIR}/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake")
EOF

# CMake configure
if ! cmake \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_TOOLCHAIN_FILE=emscripten_conan_toolchain.cmake \
  -DCMAKE_CROSSCOMPILING_EMULATOR="$EMSDK_NODE" \
  ..; then
  echo "Error: CMake configuration failed"
  exit 1
fi

# Build
if [[ "$OSTYPE" == "darwin"* ]]; then
  # macOS
  NUM_PROCESSORS="$(sysctl -n hw.ncpu)"
else
  NUM_PROCESSORS="$(nproc)"
fi

if ! cmake --build . --config "$BUILD_TYPE" -- -j "$NUM_PROCESSORS"; then
  echo "Error: Build failed"
  exit 1
fi

echo "Build completed successfully!"