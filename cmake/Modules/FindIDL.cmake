include(FindPackageHandleStandardArgs)

find_program(TAO_IDL_EXECUTABLE
  NAMES tao_idl
)

find_package_handle_standard_args(IDL
  REQUIRED_VARS TAO_IDL_EXECUTABLE
)

function(add_idl target idl_file output_dir)
  cmake_parse_arguments(ADD_IDL "NO_SERVANT;VALUETYPE" "TLBIMP" "" ${ARGN})

  get_filename_component(idl_name ${idl_file} NAME_WE)
  set(idl_source ${CMAKE_CURRENT_LIST_DIR}/${idl_file})
  set(generated_cpp ${output_dir}/${idl_name}.cpp)
  set(generated_hpp ${output_dir}/${idl_name}.hpp)
  set(generated_ipp ${output_dir}/${idl_name}.ipp)
  set(generated_servant_cpp ${output_dir}/${idl_name}_s.cpp)
  set(generated_servant_hpp ${output_dir}/${idl_name}_s.hpp)

  file(MAKE_DIRECTORY ${output_dir})

  add_custom_command(
    OUTPUT
      ${generated_cpp}
      ${generated_hpp}
      ${generated_ipp}
      ${generated_servant_cpp}
      ${generated_servant_hpp}
    COMMAND ${TAO_IDL_EXECUTABLE}
      -Sp
      -in
      -ci .ipp
      -cs .cpp
      -hc .hpp
      -hs _s.hpp
      -ss _s.cpp
      -I ${PROJECT_SOURCE_DIR}
      -I ${CORBA_INCLUDES}
      ${idl_source}
      -o ${output_dir}
    COMMAND sed -i "s/if (0 == &_tao_elem)/if (false)/g"
      ${generated_cpp}
      ${generated_servant_cpp}
    DEPENDS ${idl_source}
    VERBATIM
  )

  add_library(${target} SHARED
    ${generated_cpp}
    ${generated_servant_cpp}
  )

  target_include_directories(${target}
    PUBLIC
      ${output_dir}
  )

  target_link_libraries(${target}
    PUBLIC
      TAO::AnyTypeCode
  )

  if(NOT ADD_IDL_NO_SERVANT)
    target_link_libraries(${target}
      PUBLIC
        TAO::PortableServer
    )
  endif()

  if(ADD_IDL_VALUETYPE)
    target_link_libraries(${target}
      PUBLIC
        TAO::Valuetype
    )
  endif()

  install(TARGETS ${target} DESTINATION ${INSTALL_DIR})
endfunction()

mark_as_advanced(TAO_IDL_EXECUTABLE)
