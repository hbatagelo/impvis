project(fuzzer)

add_executable(${PROJECT_NAME} ../../src/equation.cpp fuzzer.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPTIONS_FUZZ_TESTING_TARGET})

enable_abcg(${PROJECT_NAME})
