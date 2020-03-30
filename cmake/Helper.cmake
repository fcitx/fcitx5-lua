
function(file2cstring tgt_name ifile)
  get_filename_component(ifile "${ifile}" ABSOLUTE)
  set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${tgt_name}.h")
  add_custom_command(OUTPUT "${output_file}"
    COMMAND file2cstring "${tgt_name}" "${ifile}" > "${output_file}"
    DEPENDS file2cstring ${ifile})
  add_custom_target("${tgt_name}" ALL DEPENDS "${output_file}")
endfunction()
