cmake_minimum_required(VERSION 3.16)
project(external)

add_subdirectory(abcg)
if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  add_library(${PROJECT_NAME} INTERFACE)
endif()