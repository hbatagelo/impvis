cmake_minimum_required(VERSION 3.18)

project(abcg)

set(ABCG_FILES
    abcg_application.cpp
    abcg_elapsedtimer.cpp
    abcg_exception.cpp
    abcg_image.cpp
    abcg_openglfunctions.cpp
    abcg_openglwindow.cpp
    abcg_trackball.cpp
    abcg_shaders.cpp)

add_subdirectory(third_party)

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  add_library(${PROJECT_NAME} ${ABCG_FILES})

  target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${PROJECT_NAME} PUBLIC third_party)
  target_compile_options(
    ${PROJECT_NAME}
    PUBLIC "-std=c++20"
    PUBLIC "-Oz"
    PUBLIC "-sUSE_SDL=2"
    PUBLIC "-sUSE_SDL_IMAGE=2")

  if(${CMAKE_GENERATOR} MATCHES "Ninja")
    set_target_properties(
      ${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -sSDL2_IMAGE_FORMATS="[""png""]")
  else()
    set_target_properties(
      ${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -sSDL2_IMAGE_FORMATS='["png"]')
  endif()
else()
  set(IMGUI_BINDINGS_DIR "${CMAKE_SOURCE_DIR}/bindings")
  add_library(
    ${PROJECT_NAME} ${ABCG_FILES} ${IMGUI_BINDINGS_DIR}/imgui_impl_sdl.cpp
                    ${IMGUI_BINDINGS_DIR}/imgui_impl_opengl3.cpp)

  target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_include_directories(${PROJECT_NAME} SYSTEM
                             PUBLIC ${IMGUI_BINDINGS_DIR})

  target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC third_party
    PUBLIC ${OPTIONS_TARGET})

  # Enable warnings only for selected files
  set_source_files_properties(${ABCG_FILES} PROPERTIES COMPILE_OPTIONS
                                                       "${PROJECT_WARNINGS}")

  # Use sanitizers in debug mode
  if(CMAKE_BUILD_TYPE MATCHES "DEBUG|Debug")
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SANITIZERS_TARGET})
  endif()

  target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

  if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
  endif()
endif()

# Convert binary assets to header
set(NEW_HEADER_FILE "abcg_embeddedfonts.hpp")

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${NEW_HEADER_FILE}")
  include(${CMAKE_SOURCE_DIR}/cmake/bin2h.cmake)
  set(SOURCE_FILES "assets/Inconsolata-Medium.ttf")

  message("Embedding following files into header file ${NEW_HEADER_FILE}:")

  file(WRITE ${NEW_HEADER_FILE} "#include <array>\n\n")

  foreach(file ${SOURCE_FILES})
    message("   ${file}")
    get_filename_component(variableName ${file} NAME)
    bin2h(SOURCE_FILE ${file} HEADER_FILE ${NEW_HEADER_FILE} VARIABLE_NAME
          ${variableName})
    file(APPEND ${NEW_HEADER_FILE} "\n")
  endforeach()
endif()
