cmake_minimum_required(VERSION 3.21)

project(imgui)

set(IMGUI_FILES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_impl_sdl.cpp
    imgui_impl_opengl3.cpp)

add_library(${PROJECT_NAME} ${IMGUI_FILES})

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE /usr/local/include /opt/local/include
  PRIVATE ${SDL2_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-s USE_SDL=2")
else()
  find_package(SDL2 REQUIRED)
  target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARY}
                                                ${CMAKE_DL_LIBS})
endif()

if(APPLE)
  add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW)
  target_include_directories(${PROJECT_NAME} PRIVATE /usr/local/include
                                                     /opt/local/include)
endif()
