
set(IDL_FOUND TRUE)
#/usr/bin/protoc --proto_path=../../../../Frontends/Modules/BiddingFrontend --cpp_out=.  ../../../../Frontends/Modules/BiddingFrontend/tanx-bidding.proto
function(add_pb _target _pbfile target_dir)
    get_filename_component(IDL_FILE_NAME_WE ${_pbfile} NAME_WE)
    set(MIDL_OUTPUT_PATH ${target_dir})
    set(OUTPUTCPP ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.pb.cc)

    include_directories(${target_dir})


    
    file(MAKE_DIRECTORY ${target_dir})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${target_dir})
    set (SRC ${CMAKE_CURRENT_LIST_DIR}/${_pbfile})
    set(PROTOBUFF_TARGET ${_target}_pb)
    add_custom_target(${PROTOBUFF_TARGET} DEPENDS "${OUTPUTCPP}" )
    add_custom_command(
       OUTPUT  ${OUTPUTCPP} ${OUTPUTHPP}

       COMMAND protoc --proto_path=${CMAKE_CURRENT_LIST_DIR} --cpp_out=${MIDL_OUTPUT_PATH}   ${SRC}
      DEPENDS ${PROTOBUFF_TARGET}
    )

 
    add_library(${_target} STATIC
	${OUTPUTCPP}
    )    
    add_dependencies(${_target} ${PROTOBUFF_TARGET})
    target_include_directories(${_target} INTERFACE ${MIDL_OUTPUT_PATH})
endfunction()