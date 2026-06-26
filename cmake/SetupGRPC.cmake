function(add_grpc_sources target proto_file output_dir)
  get_filename_component(proto_name_we ${proto_file} NAME_WE)
  get_filename_component(proto_abs ${proto_file} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  get_filename_component(proto_dir ${proto_abs} DIRECTORY)

  set(pb_cc ${output_dir}/${proto_name_we}.pb.cc)
  set(pb_h ${output_dir}/${proto_name_we}.pb.h)
  set(grpc_cc ${output_dir}/${proto_name_we}.grpc.pb.cc)
  set(grpc_h ${output_dir}/${proto_name_we}.grpc.pb.h)

  file(MAKE_DIRECTORY ${output_dir})

  add_custom_command(
    OUTPUT ${pb_cc} ${pb_h} ${grpc_cc} ${grpc_h}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    --experimental_editions
    --proto_path=${proto_dir}
    ${ARGN}
    --cpp_out=${output_dir}
    --grpc_out=${output_dir}
    --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    ${proto_abs}
    DEPENDS ${proto_abs}
  )

  if (TARGET ${target})
    target_sources(${target} PRIVATE ${pb_cc} ${grpc_cc})
  else()
    add_library(${target} STATIC ${pb_cc} ${grpc_cc})
  endif()

  target_include_directories(${target} PUBLIC ${output_dir})
endfunction()

function(add_adserver_grpc_client_sources target proto_file output_dir namespace)
  execute_process(
    COMMAND /usr/bin/env python3.8 -c "import google.protobuf.compiler.plugin_pb2"
    RESULT_VARIABLE adserver_grpc_client_protobuf_result
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(NOT adserver_grpc_client_protobuf_result EQUAL 0)
    message(FATAL_ERROR
      "adserver grpc client generator requires python3.8 with google.protobuf "
      "(for example package python38-protobuf or python3-protobuf)")
  endif()

  get_filename_component(proto_name_we ${proto_file} NAME_WE)
  get_filename_component(proto_abs ${proto_file} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  get_filename_component(proto_dir ${proto_abs} DIRECTORY)
  string(REPLACE "::" "." namespace_param "${namespace}")

  set(client_cc ${output_dir}/${proto_name_we}.grpc-client.cpp)
  set(client_h ${output_dir}/${proto_name_we}.grpc-client.hpp)

  file(MAKE_DIRECTORY ${output_dir})

  add_custom_command(
    OUTPUT ${client_cc} ${client_h}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    --experimental_editions
    --proto_path=${proto_dir}
    ${ARGN}
    --plugin=protoc-gen-adserver-grpc-client=${CMAKE_SOURCE_DIR}/cmake/protoc-gen-adserver-grpc-client.py
    --adserver-grpc-client_out=namespace=${namespace_param}:${output_dir}
    ${proto_abs}
    DEPENDS
    ${proto_abs}
    ${CMAKE_SOURCE_DIR}/cmake/protoc-gen-adserver-grpc-client.py
  )

  target_sources(${target} PRIVATE ${client_cc})
  target_include_directories(${target} PUBLIC ${output_dir})
endfunction()
