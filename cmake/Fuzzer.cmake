# Enable fuzz testing for target project_target
function(enable_fuzzer project_target)

  set(LIST_OF_SANITIZERS "fuzzer,address,undefined")

  target_compile_options(${project_target}
                         INTERFACE -O2 -g -fsanitize=${LIST_OF_SANITIZERS})
  target_link_options(${project_target} INTERFACE
                      -fsanitize=${LIST_OF_SANITIZERS})

endfunction()
