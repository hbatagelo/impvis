add_executable(${PROJECT_NAME} main.cpp background.cpp textureblit.cpp
                               equation.cpp window.cpp raycast.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE ${PROJECT_WARNINGS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPTIONS_TARGET})

enable_abcg(${PROJECT_NAME})

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  target_link_libraries(${PROJECT_NAME} PRIVATE external)
endif()
