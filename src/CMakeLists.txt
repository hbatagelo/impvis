add_executable(
  ${PROJECT_NAME}
  arrow.cpp
  axes.cpp
  background.cpp
  camera.cpp
  function.cpp
  functionmanager.cpp
  geometry.cpp
  main.cpp
  raycast.cpp
  renderpipeline.cpp
  rendertarget.cpp
  swapchain.cpp
  textureblit.cpp
  ui.cpp
  ui_editor.cpp
  ui_legends.cpp
  ui_tabs.cpp
  ui_widgets.cpp
  window.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE ${PROJECT_WARNINGS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPTIONS_TARGET})

enable_abcg(${PROJECT_NAME})

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  target_link_libraries(${PROJECT_NAME} PRIVATE embind)
endif()
