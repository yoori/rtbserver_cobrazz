include(FindPackageHandleStandardArgs)

find_program(XSDCXX_EXECUTABLE
  NAMES xsdcxx
)

find_package_handle_standard_args(XSD
  REQUIRED_VARS XSDCXX_EXECUTABLE
)

function(add_xsd _target _xsdfile target_dir)
  get_filename_component(IDL_FILE_NAME_WE ${_xsdfile} NAME_WE)
  set(MIDL_OUTPUT_PATH ${target_dir})
  set(OUTPUTCPP ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.cpp)
  set(OUTPUTHPP ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.hpp)

  include_directories(${target_dir})

  file(MAKE_DIRECTORY ${target_dir})
  set(SRC ${CMAKE_CURRENT_LIST_DIR}/${_xsdfile})
  add_custom_command(
    OUTPUT ${OUTPUTCPP} ${OUTPUTHPP}
    COMMAND ${XSDCXX_EXECUTABLE}
      cxx-tree
      --std c++11
      --hxx-suffix .hpp
      --ixx-suffix .ipp
      --cxx-suffix .cpp
      --namespace-regex "|^XMLSchema.xsd http://www.w3.org/2001/XMLSchema$|xml_schema|"
      --fwd-suffix -fwd.hpp
      --output-dir ${target_dir}
      ${SRC}
    DEPENDS ${SRC}
    VERBATIM
  )

  add_library(${_target} STATIC
    ${OUTPUTCPP}
  )
  target_link_libraries(${_target}
    PUBLIC
      XercesC::XercesC
  )
  target_include_directories(${_target}
    PUBLIC
      ${MIDL_OUTPUT_PATH}
  )
endfunction()

mark_as_advanced(XSDCXX_EXECUTABLE)
