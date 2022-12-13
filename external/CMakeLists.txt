cmake_minimum_required(VERSION 3.11)

project(external)

add_subdirectory(abcg)

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  add_library(${PROJECT_NAME} INTERFACE)

  option(RE2_BUILD_TESTING OFF)
  add_subdirectory(re2)
  add_subdirectory(tomlplusplus)

  target_link_libraries(${PROJECT_NAME} INTERFACE re2::re2 tomlplusplus)
endif()
