include(FindPackageHandleStandardArgs)

if(NOT TARGET ACE::ACE)
  find_package(ACE REQUIRED)
endif()

find_path(TAO_INCLUDE_DIR NAMES tao/corba.h)
find_library(TAO_LIBRARY NAMES TAO)
find_library(TAO_ANY_TYPE_CODE_LIBRARY NAMES TAO_AnyTypeCode)
find_library(TAO_CODEC_FACTORY_LIBRARY NAMES TAO_CodecFactory)
find_library(TAO_ENDPOINT_POLICY_LIBRARY NAMES TAO_EndpointPolicy)
find_library(TAO_IOR_TABLE_LIBRARY NAMES TAO_IORTable)
find_library(TAO_MESSAGING_LIBRARY NAMES TAO_Messaging)
find_library(TAO_PORTABLE_SERVER_LIBRARY NAMES TAO_PortableServer)
find_library(TAO_SECURITY_LIBRARY NAMES TAO_Security)
find_library(TAO_TC_IIOP_LIBRARY NAMES TAO_TC_IIOP)
find_library(TAO_VALUETYPE_LIBRARY NAMES TAO_Valuetype)

find_package_handle_standard_args(TAO
  REQUIRED_VARS
    TAO_INCLUDE_DIR
    TAO_LIBRARY
    TAO_ANY_TYPE_CODE_LIBRARY
    TAO_CODEC_FACTORY_LIBRARY
    TAO_ENDPOINT_POLICY_LIBRARY
    TAO_IOR_TABLE_LIBRARY
    TAO_MESSAGING_LIBRARY
    TAO_PORTABLE_SERVER_LIBRARY
    TAO_SECURITY_LIBRARY
    TAO_TC_IIOP_LIBRARY
    TAO_VALUETYPE_LIBRARY
)

if(TAO_FOUND)
  if(NOT TARGET TAO::TAO)
    add_library(TAO::TAO UNKNOWN IMPORTED)
    set_target_properties(TAO::TAO PROPERTIES
      IMPORTED_LOCATION "${TAO_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TAO_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES ACE::ACE
    )
  endif()

  if(NOT TARGET TAO::AnyTypeCode)
    add_library(TAO::AnyTypeCode UNKNOWN IMPORTED)
    set_target_properties(TAO::AnyTypeCode PROPERTIES
      IMPORTED_LOCATION "${TAO_ANY_TYPE_CODE_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TAO_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES TAO::TAO
    )
  endif()

  if(NOT TARGET TAO::PortableServer)
    add_library(TAO::PortableServer UNKNOWN IMPORTED)
    set_target_properties(TAO::PortableServer PROPERTIES
      IMPORTED_LOCATION "${TAO_PORTABLE_SERVER_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TAO_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES TAO::AnyTypeCode
    )
  endif()

  if(NOT TARGET TAO::Valuetype)
    add_library(TAO::Valuetype UNKNOWN IMPORTED)
    set_target_properties(TAO::Valuetype PROPERTIES
      IMPORTED_LOCATION "${TAO_VALUETYPE_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TAO_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES TAO::AnyTypeCode
    )
  endif()

  function(_tao_import_component _component _library)
    if(NOT TARGET TAO::${_component})
      add_library(TAO::${_component} UNKNOWN IMPORTED)
      set_target_properties(TAO::${_component} PROPERTIES
        IMPORTED_LOCATION "${_library}"
        INTERFACE_INCLUDE_DIRECTORIES "${TAO_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES TAO::TAO
      )
    endif()
  endfunction()

  _tao_import_component(CodecFactory "${TAO_CODEC_FACTORY_LIBRARY}")
  _tao_import_component(EndpointPolicy "${TAO_ENDPOINT_POLICY_LIBRARY}")
  _tao_import_component(IORTable "${TAO_IOR_TABLE_LIBRARY}")
  _tao_import_component(Messaging "${TAO_MESSAGING_LIBRARY}")
  _tao_import_component(Security "${TAO_SECURITY_LIBRARY}")
  _tao_import_component(TC_IIOP "${TAO_TC_IIOP_LIBRARY}")
endif()

mark_as_advanced(
  TAO_INCLUDE_DIR
  TAO_LIBRARY
  TAO_ANY_TYPE_CODE_LIBRARY
  TAO_CODEC_FACTORY_LIBRARY
  TAO_ENDPOINT_POLICY_LIBRARY
  TAO_IOR_TABLE_LIBRARY
  TAO_MESSAGING_LIBRARY
  TAO_PORTABLE_SERVER_LIBRARY
  TAO_SECURITY_LIBRARY
  TAO_TC_IIOP_LIBRARY
  TAO_VALUETYPE_LIBRARY
)
