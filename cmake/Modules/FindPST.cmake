
set(IDL_FOUND TRUE)

function(add_pst _target _pstfile target_dir)
    get_filename_component(IDL_FILE_NAME_WE ${_pstfile} NAME_WE)
    set(MIDL_OUTPUT_PATH ${target_dir})
#    set(OUTPUTCPP ${MIDL_OUTPUT_PATH}/${_pstfile}.cpp)
#    set(OUTPUTHPP ${MIDL_OUTPUT_PATH}/${_pstfile}.hpp)
    set(OUTPUTCPP ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.cpp)
    set(OUTPUTHPP ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.hpp)

    include_directories(${target_dir})


    
    file(MAKE_DIRECTORY ${target_dir})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${target_dir})
    set (SRC ${CMAKE_CURRENT_LIST_DIR}/${_pstfile})
    set(PST_TARGET ${_target}_pst)
    add_custom_target(${PST_TARGET} DEPENDS "${OUTPUTCPP}" )
    add_custom_command(
       OUTPUT  ${OUTPUTCPP} ${OUTPUTHPP}

       COMMAND PlainCppCompiler --output-hpp=${OUTPUTHPP} --output-cpp=${OUTPUTCPP} ${SRC}
#PlainCppCompiler -output-hpp=SegmentProfile.hpp --output-cpp=SegmentProfile.cpp       
      DEPENDS ${PST_TARGET}
    )

 
    add_library(${_target} STATIC
	${OUTPUTCPP}
    )    
    add_dependencies(${_target} ${PST_TARGET})
    target_include_directories(${_target} INTERFACE ${MIDL_OUTPUT_PATH})
endfunction()