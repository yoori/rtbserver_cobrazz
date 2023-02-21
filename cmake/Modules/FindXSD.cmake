
set(IDL_FOUND TRUE)

function(add_xsd _target _xsdfile target_dir)
    get_filename_component(IDL_FILE_NAME_WE ${_xsdfile} NAME_WE)
    set(MIDL_OUTPUT_PATH ${target_dir})
    set(OUTPUTCPP ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.cpp)
    set(OUTPUTHPP ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.hpp)

    include_directories(${target_dir})

    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(MIDL_ARCH win32)
    else()
        set(MIDL_ARCH x64)
    endif()

    
    file(MAKE_DIRECTORY ${target_dir})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${target_dir})
    set (SRC ${CMAKE_CURRENT_LIST_DIR}/${_xsdfile})
    set(FINDIDL_TARGET ${_target}_gen)
    add_custom_target(${FINDIDL_TARGET} DEPENDS ${OUTPUTCPP}  ${OUTPUTHPP})
    add_custom_command(
       OUTPUT  ${OUTPUTCPP} ${OUTPUTHPP}

       COMMAND /usr/bin/xsdcxx ARGS cxx-tree --std c++11 --hxx-suffix .hpp --ixx-suffix .ipp --cxx-suffix .cpp  --namespace-regex  "\"|^XMLSchema.xsd http://www.w3.org/2001/XMLSchema$$|xml_schema|\""   --fwd-suffix -fwd.hpp --output-dir ${target_dir}  ${SRC}
    DEPENDS ${FINDIDL_TARGET}
    )

 
    add_library(${_target} STATIC
	${OUTPUTCPP}
    )    
    add_dependencies(${_target} ${FINDIDL_TARGET})
#    add_dependencies(${_target} ${OUTPUTCPP})
#    add_dependencies(${_target} ${OUTPUTHPP})
    target_include_directories(${_target} INTERFACE ${MIDL_OUTPUT_PATH})
endfunction()