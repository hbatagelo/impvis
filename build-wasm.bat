@echo off

set BUILD_TYPE=Release

:: Reset build directory
rd /s /q build 2>nul
mkdir build & cd build

:: Create a WASM profile
(
echo [settings]
echo arch=wasm
echo build_type=Release
echo compiler=clang
echo compiler.cppstd=20
echo compiler.libcxx=libc++
echo compiler.version=21
echo os=Emscripten
echo.
echo [buildenv]
echo CC=emcc
echo CXX=em++
echo AR=emar
echo RANLIB=emranlib
echo.
echo [options]
echo *:opengl=False
echo *:shared=False
echo *:fPIC=False
echo.
echo [conf]
echo tools.cmake.cmaketoolchain:generator=Ninja
echo tools.build:compiler_executables={"c": "emcc", "cpp": "em++"}
echo tools.meson.mesontoolchain:backend=ninja
) > wasm_profile

:: Install dependencies with Conan
conan install .. ^
  --profile:build=default ^
  --profile:host=wasm_profile ^
  -b missing ^
  -s build_type=%BUILD_TYPE%

if errorlevel 1 (
    echo Conan install failed
    cd ..
    exit /b 1
)

:: Create wrapper toolchain that chains both toolchains
(
echo # Load Emscripten toolchain first
echo include^("$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"^)
echo.
echo # Then load Conan toolchain
echo include^("${CMAKE_CURRENT_LIST_DIR}/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake"^)
) > emscripten_conan_toolchain.cmake

:: CMake configure
cmake ^
  -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
  -DCMAKE_TOOLCHAIN_FILE=emscripten_conan_toolchain.cmake ^
  -DCMAKE_CROSSCOMPILING_EMULATOR=%EMSDK_NODE% ^
  ..

if errorlevel 1 (
    echo CMake configuration failed
    cd ..
    exit /b 1
)

:: Build
cmake --build . --config %BUILD_TYPE% -- -j %NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo Build failed
    cd ..
    exit /b 1
)

cd ..
echo Build completed successfully!