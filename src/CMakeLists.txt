set(IMPVIS_FILES main.cpp background.cpp textureblit.cpp equation.cpp
                 window.cpp raycast.cpp)

add_executable(${PROJECT_NAME} ${IMPVIS_FILES})

set_source_files_properties(${IMPVIS_FILES} PROPERTIES COMPILE_OPTIONS
                                                       "${PROJECT_WARNINGS}")

target_link_libraries(${PROJECT_NAME} PUBLIC ${OPTIONS_TARGET})

enable_abcg(${PROJECT_NAME})

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  target_link_libraries(${PROJECT_NAME} PUBLIC external)
endif()
