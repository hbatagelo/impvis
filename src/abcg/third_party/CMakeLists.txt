cmake_minimum_required(VERSION 3.11)

project(third_party)

add_library(${PROJECT_NAME} INTERFACE)

add_subdirectory(tiny_obj_loader)

target_include_directories(${PROJECT_NAME} SYSTEM
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} INTERFACE tiny_obj_loader)

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  add_subdirectory(imgui)
  add_subdirectory(cppitertools)
  add_subdirectory(fmt)
  add_subdirectory(glm)
  add_subdirectory(gsl)

  target_include_directories(
    ${PROJECT_NAME} SYSTEM
    INTERFACE ${GLEW_INCLUDE_DIRS}
    INTERFACE ${OPENGL_INCLUDE_DIR}
    INTERFACE ${SDL2_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} INTERFACE cppitertools fmt glm gsl
                                                  imgui)
endif()
