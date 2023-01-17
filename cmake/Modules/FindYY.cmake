
set(IDL_FOUND TRUE)

function(add_yy _target _yyfile target_dir)
    get_filename_component(IDL_FILE_NAME_WE ${_yyfile} NAME_WE)
    set(MIDL_OUTPUT_PATH ${target_dir})
    set(OUTPUTCPP ${MIDL_OUTPUT_PATH}/${_yyfile}.cpp)

    include_directories(${target_dir})


    
    file(MAKE_DIRECTORY ${target_dir})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${target_dir})
    set (SRC ${CMAKE_CURRENT_LIST_DIR}/${_yyfile})
    set(BISON_TARGET ${_target}_yy)
    add_custom_target(${BISON_TARGET} DEPENDS "${OUTPUTCPP}" )
    add_custom_command(
       OUTPUT  ${OUTPUTCPP} ${OUTPUTHPP}

       COMMAND bison -d --report=state -o ${OUTPUTCPP} ${SRC}
      DEPENDS ${BISON_TARGET}
    )

 
    add_library(${_target} STATIC
	${OUTPUTCPP}
    )    
    add_dependencies(${_target} ${BISON_TARGET})
    target_include_directories(${_target} INTERFACE ${MIDL_OUTPUT_PATH})
endfunction()