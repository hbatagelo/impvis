function(enable_abcg project_target)

  if(ARGC GREATER 1)
    # If a second argument is passed, it is treated as a string containing extra
    # libraries to be linked with
    target_link_libraries(${project_target} PUBLIC abcg ${ARGV1})
  else()
    target_link_libraries(${project_target} PUBLIC abcg)
  endif()

  if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    target_compile_options(
      ${project_target}
      PUBLIC ${PROJECT_WARNINGS}
      PUBLIC "-std=c++20"
      PUBLIC "-Oz"
      PUBLIC "-sUSE_SDL=3")

    set(LINK_FLAGS "")
    # For debugging, use ASSERTIONS=1 and DISABLE_EXCEPTION_CATCHING=0
    list(APPEND LINK_FLAGS "-sASSERTIONS=0")
    list(APPEND LINK_FLAGS "-sDISABLE_EXCEPTION_CATCHING=1")
    list(APPEND LINK_FLAGS "-sALLOW_MEMORY_GROWTH=1")
    list(APPEND LINK_FLAGS "-sFULL_ES3=1")
    list(APPEND LINK_FLAGS "-sMAX_WEBGL_VERSION=2")
    list(APPEND LINK_FLAGS "-sMIN_WEBGL_VERSION=2")
    list(APPEND LINK_FLAGS "-sUSE_SDL=3")
    list(APPEND LINK_FLAGS "-sWASM=1")
    list(APPEND LINK_FLAGS "-sSTACK_SIZE=192KB")
    list(APPEND LINK_FLAGS "--use-preload-plugins")
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
      list(APPEND LINK_FLAGS
           "--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets ")
    endif()
    string(REPLACE ";" " " LINK_FLAGS "${LINK_FLAGS}")

    set_target_properties(
      ${project_target}
      PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/public"
                 LINK_FLAGS "${LINK_FLAGS}")
  else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(
      ${project_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    get_target_property(output_dir ${project_target} RUNTIME_OUTPUT_DIRECTORY)

    # Building from Visual Studio IDE
    if(MSVC AND ${output_dir} MATCHES "/out/build/")
      # Copy assets directory to ${output_dir}
      if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
        add_custom_command(
          TARGET ${project_target}
          POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_directory
                  ${CMAKE_CURRENT_SOURCE_DIR}/assets ${output_dir}/assets)
      endif()

      # Copy SDL3 DLLs: extract first string delimited by ';', extract path then
      # copy
      list(GET SDL3_LIBRARY 0 SDL3_FIRSTPATH)
      string(REGEX REPLACE "(.+\/).+\.lib" "\\1" SDL3_LIBPATH ${SDL3_FIRSTPATH})
      file(GLOB SDL3_DLLS "${SDL3_LIBPATH}*.dll")
      file(COPY ${SDL3_DLLS} DESTINATION ${output_dir})
    else()
      # POST_BUILD: copy executable and assets to bin directory
      if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(extension ".exe")
      endif()

      # Copy assets directory to ${output_dir}
      if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
        add_custom_command(
          TARGET ${project_target}
          POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_directory
                  ${CMAKE_CURRENT_SOURCE_DIR}/assets ${output_dir}/assets)
      endif()

      # Take into account that, on Windows with MSVC, binaries are placed in a
      # subdirectory named after the build type
      set(build_type "")
      if(MSVC)
        string(
          APPEND build_type "$<$<CONFIG:Debug>:Debug/>"
          "$<$<CONFIG:Release>:Release/>" "$<$<CONFIG:MinSizeRel>:MinSizeRel/>"
          "$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo/>")

        # Copy SDL3 DLLs: extract first string delimited by ';', extract path
        # then copy
        list(GET SDL3_LIBRARY 0 SDL3_FIRSTPATH)
        string(REGEX REPLACE "(.+\/).+\.lib" "\\1" SDL3_LIBPATH
                             ${SDL3_FIRSTPATH})
        file(GLOB SDL3_DLLS "${SDL3_LIBPATH}*.dll")
        file(COPY ${SDL3_DLLS} DESTINATION ${output_dir}/)
      endif()

      add_custom_command(
        TARGET ${project_target}
        POST_BUILD
        COMMAND
          # Copy executable from output directory to ${project_target}
          ${CMAKE_COMMAND} -E copy
          ${output_dir}/${build_type}${project_target}${extension}
          ${output_dir}/${project_target}${extension})
    endif()

  endif()

endfunction()
