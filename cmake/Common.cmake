# Generate compile_commands.json to make it easier to work with clang based
# tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  set(SANITIZERS_TARGET sanitizers)
  set(WARNINGS_TARGET warnings)
  set(OPTIONS_TARGET options)
  add_library(${SANITIZERS_TARGET} INTERFACE)
  add_library(${WARNINGS_TARGET} INTERFACE)
  add_library(${OPTIONS_TARGET} INTERFACE)

  if(NOT MSVC)
    option(ENABLE_MOLD "Enable mold (Modern Linker)" ON)
  endif()

  if(NOT ENABLE_MOLD)
    option(ENABLE_IPO "Enable Interprocedural Optimization" ON)
  endif()

  option(ENABLE_UNIT_TESTING "Enable unit testing" OFF)

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    option(ENABLE_FUZZ_TESTING "Enable fuzz testing" OFF)
  endif()

  if(ENABLE_UNIT_TESTING)
    set(OPTIONS_UNIT_TESTING_TARGET options_unit_testing)
    add_library(${OPTIONS_UNIT_TESTING_TARGET} INTERFACE)
  endif()

  if(ENABLE_FUZZ_TESTING)
    set(OPTIONS_FUZZ_TESTING_TARGET options_fuzz_testing)
    add_library(${OPTIONS_FUZZ_TESTING_TARGET} INTERFACE)

    include(${CMAKE_CURRENT_LIST_DIR}/Fuzzer.cmake)
    enable_fuzzer(${OPTIONS_FUZZ_TESTING_TARGET})
  endif()

  # Standard compiler warnings
  include(${CMAKE_CURRENT_LIST_DIR}/Warnings.cmake)
  set_project_warnings(${WARNINGS_TARGET} PROJECT_WARNINGS)

  # Sanitizer options if supported by compiler
  include(${CMAKE_CURRENT_LIST_DIR}/Sanitizers.cmake)
  enable_sanitizers(${SANITIZERS_TARGET})
endif()

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui, ccmake
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()

# Conan
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  find_package(fmt)
  find_package(imgui)
  find_package(cppitertools)
  find_package(glm)
  find_package(Microsoft.GSL)
  find_package(GLEW)
  find_package(re2)
  find_package(tomlplusplus)
  find_package(SDL2)
  find_package(SDL2_image)

  # C++20
  target_compile_features(${OPTIONS_TARGET} INTERFACE cxx_std_20)

  # Conan libs
  target_link_libraries(
    ${OPTIONS_TARGET}
    INTERFACE fmt::fmt
              imgui::imgui
              cppitertools::cppitertools
              GLEW::GLEW
              glm::glm
              Microsoft.GSL::GSL
              re2::re2
              SDL2::SDL2
              SDL2_image::SDL2_image
              tomlplusplus::tomlplusplus)

  if(ENABLE_UNIT_TESTING)
    find_package(GTest)
    target_link_libraries(
      ${OPTIONS_UNIT_TESTING_TARGET}
      INTERFACE GTest::gtest GTest::gtest_main ${OPTIONS_TARGET}
                ${SANITIZERS_TARGET})
  endif()
endif()

# mold
if(ENABLE_MOLD)
  find_program(MOLD mold)
  if(MOLD)
    message("Using mold")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
  else()
    message("Not using mold - not found")
  endif()
endif()

# IPO
if(ENABLE_IPO)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.9)
    cmake_policy(SET CMP0069 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
  endif()

  include(CheckIPOSupported)
  check_ipo_supported(RESULT result OUTPUT output)

  if(result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    if(CMAKE_COMPILER_IS_GNUCXX)
      list(APPEND CMAKE_C_COMPILE_OPTIONS_IPO "-flto=auto")
      list(APPEND CMAKE_CXX_COMPILE_OPTIONS_IPO "-flto=auto")
    endif()
  else()
    message("IPO is not supported: ${output}")
  endif()
endif()

# ccache
if(NOT MSVC)
  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten" OR NOT ${CMAKE_GENERATOR}
                                                      MATCHES "Ninja")
    find_program(CCACHE ccache)
    if(CCACHE)
      message("Using ccache")
      set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    else()
      message("Not using ccache - not found")
    endif()
  else()
    message("Not using ccache - disabled for Ninja builds with Emscripten")
  endif()
elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  # Silence warnings about linking the release DLL version of the CRT
  # Third-party libraries are always compiled in release mode
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
endif()

# ABCg
include(${CMAKE_CURRENT_LIST_DIR}/ABCg.cmake)
