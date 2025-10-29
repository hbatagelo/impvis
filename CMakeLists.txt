if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  set(CONAN_PROVIDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan_provider.cmake")
  set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CONAN_PROVIDER_PATH}")
endif()

cmake_minimum_required(VERSION 3.24)

project(impvis)

include(cmake/Common.cmake)

add_subdirectory(external)
add_subdirectory(src)

if(ENABLE_UNIT_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

if(ENABLE_FUZZ_TESTING)
  add_subdirectory(tests/fuzzer)
endif()
