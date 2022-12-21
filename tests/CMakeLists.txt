project(tests)

add_executable(${PROJECT_NAME} equation_test.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPTIONS_UNIT_TESTING_TARGET})

enable_abcg(${PROJECT_NAME})

add_test(NAME Test COMMAND ${PROJECT_NAME})
