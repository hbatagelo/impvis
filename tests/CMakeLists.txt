project(tests)

add_library(function_testable STATIC "${CMAKE_SOURCE_DIR}/src/function.cpp")

target_include_directories(function_testable PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_compile_definitions(function_testable PRIVATE ENABLE_UNIT_TESTING)
target_link_libraries(function_testable PRIVATE abcg ${OPTIONS_TARGET})

add_executable(${PROJECT_NAME} function_test.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(${PROJECT_NAME} PRIVATE function_testable
                                              ${OPTIONS_UNIT_TESTING_TARGET})
enable_abcg(${PROJECT_NAME})

add_test(NAME Test COMMAND ${PROJECT_NAME})
