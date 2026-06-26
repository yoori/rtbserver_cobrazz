
set(IDL_FOUND TRUE)

function(add_pb _target _pbfile target_dir)
  get_filename_component(IDL_FILE_NAME_WE ${_pbfile} NAME_WE)
  set(MIDL_OUTPUT_PATH ${target_dir})
  set(OUTPUTCPP ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.pb.cc)

  include_directories(${target_dir})

  file(MAKE_DIRECTORY ${target_dir})
  execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${target_dir})
  set (SRC ${CMAKE_CURRENT_LIST_DIR}/${_pbfile})
  set(PROTOBUF_TARGET ${_target}_pb)
  add_custom_target(${PROTOBUF_TARGET} DEPENDS "${OUTPUTCPP}")
  add_custom_command(
    OUTPUT ${OUTPUTCPP} ${OUTPUTHPP}
    COMMAND protoc
      --experimental_editions
      --proto_path=${CMAKE_CURRENT_LIST_DIR}
      --cpp_out=${MIDL_OUTPUT_PATH}
      ${SRC}
    DEPENDS ${PROTOBUF_TARGET}
  )

  add_library(${_target} STATIC ${OUTPUTCPP})
  add_dependencies(${_target} ${PROTOBUF_TARGET})
  target_include_directories(${_target} INTERFACE ${MIDL_OUTPUT_PATH})
  target_link_libraries(${_target}
    protobuf::libprotobuf
    # Workaround for abseil invalid export for shared libraries
    absl_log_internal_check_op
    absl_log_internal_message
  )
endfunction()
