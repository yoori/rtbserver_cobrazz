# Redistribution and use is allowed under the OSI-approved 3-clause BSD license.
# Copyright (c) 2018 Apriorit Inc. All rights reserved.

set(IDL_FOUND TRUE)

##hash table start
# set key-value
function(set_key_value array_name key value)
    set("${array_name}_${key}" "${value}" PARENT_SCOPE)
endfunction()

# get value by key
function(get_value array_name key result)
    set("${result}" "${${array_name}_${key}}" PARENT_SCOPE)
endfunction()

# delete key-value
function(remove_key array_name key)
    unset("${array_name}_${key}" PARENT_SCOPE)
endfunction()

# iterate over keys
function(iterate_keys array_name)
    foreach(key IN LISTS "${array_name}_keys")
        message("Key: ${key}")
        get_value(${array_name} ${key} value)
        message("Value: ${value}")
    endforeach()
endfunction()
## example of all usages
#set_key_value(my_array foo bar)
#set_key_value(my_array hello world)
## get value
#get_value(my_array foo result)
#message("Value of foo: ${result}")
## delete key
#remove_key(my_array hello)
## try to get deleted value
#get_value(my_array hello result)
#message("Value of hello: ${result}")
## key iteration
# iterate_keys(my_array)
##hash table end

#create hash table for idl and their namespaces and exclude classes and special namespace for class
set_key_value(idlAndTheirNamespaces CorbaTypesIDL CORBACommons)

set_key_value(idlAndTheirNamespaces CampaignCommonsIDL AdServer::CampaignSvcs_v360)
set_key_value(idlAndTheirNamespaces CampaignCommons_v350IDL AdServer::CampaignSvcs_v350)

set_key_value(idlAndTheirNamespaces BillStatInfoIDL AdServer::CampaignSvcs_v360)
set_key_value(idlAndTheirNamespaces BillStatInfo_v350IDL AdServer::CampaignSvcs_v350)

set_key_value(idlAndTheirNamespaces StatInfoIDL AdServer::CampaignSvcs_v360)
set_key_value(idlAndTheirNamespaces StatInfo_v350IDL AdServer::CampaignSvcs_v350)

set_key_value(idlAndTheirNamespaces CampaignServerIDL AdServer::CampaignSvcs_v360)
set_key_value(excludeClasses CampaignServerIDL "ImplementationException NotReady NotSupport CampaignServer")
set_key_value(idlAndTheirNamespaces CampaignServer_v350IDL AdServer::CampaignSvcs_v350)
set_key_value(excludeClasses CampaignServer_v350IDL "ImplementationException NotReady NotSupport CampaignServer")

set_key_value(idlAndTheirNamespaces UserInfoExchangerIDL AdServer::UserInfoSvcs)
set_key_value(excludeClasses UserInfoExchangerIDL "ImplementationException NotReady UserInfoExchanger")

set_key_value(idlAndTheirNamespaces CampaignManagerIDL AdServer::CampaignSvcs_v360::CampaignManager)
set_key_value(excludeClasses CampaignManagerIDL "ImplementationException IncorrectArgument NotReady CampaignManager")

set_key_value(idlAndTheirNamespaces LogGeneralizerIDL AdServer::LogProcessing)
set_key_value(excludeClasses LogGeneralizerIDL "ImplementationException NotSupported CollectionNotStarted LogGeneralizer")

set_key_value(idlAndTheirNamespaces ChannelSearchServiceIDL AdServer::ChannelSearchSvcs)
set_key_value(excludeClasses ChannelSearchServiceIDL "ImplementationException ChannelSearch")

set_key_value(idlAndTheirNamespaces UserInfoManagerIDL AdServer::UserInfoSvcs::UserInfoMatcher)
set_key_value(excludeClasses UserInfoManagerIDL "ImplementationException NotReady ChunkNotFound UserInfoMatcher UserInfoManager")

set_key_value(idlAndTheirNamespaces UserInfoManagerControlIDL AdServer::UserInfoSvcs)
set_key_value(excludeClasses UserInfoManagerControlIDL "ImplementationException UserInfoManagerControl")

set_key_value(idlAndTheirNamespaces UserBindServerIDL AdServer::UserInfoSvcs::UserBindServer)
set_key_value(excludeClasses UserBindServerIDL "UserBindMapper UserBindServer")

set_key_value(idlAndTheirNamespaces UserBindControllerIDL AdServer::UserInfoSvcs)
set_key_value(excludeClasses UserBindControllerIDL "UserBindMapperValueType UserBindClusterControl NotReady ImplementationException UserBindController")

set_key_value(idlAndTheirNamespaces ChannelClusterControlIDL AdServer::ChannelSvcs)
set_key_value(excludeClasses ChannelClusterControlIDL "ChannelClusterSession ImplementationException ChannelClusterControl")

set_key_value(idlAndTheirNamespaces ChannelManagerControllerIDL AdServer::ChannelSvcs::Protected)
set_key_value(excludeClasses ChannelManagerControllerIDL "ChannelServerSession ChannelLoadSessionBase ChannelManagerController")

set_key_value(idlAndTheirNamespaces ChannelUpdateBaseIDL AdServer::ChannelSvcs)
set_key_value(excludeClasses ChannelUpdateBaseIDL "ChannelLoadSession")
set_key_value(specialNamespaceForClass ChannelUpdateBaseIDL "TriggerInfoSeq=AdServer::ChannelSvcs::ChannelUpdateBase_v33")

set_key_value(idlAndTheirNamespaces ChannelUpdate_v28IDL AdServer::ChannelSvcs)
set_key_value(excludeClasses ChannelUpdate_v28IDL "ChannelLoadSession")
set_key_value(specialNamespaceForClass ChannelUpdate_v28IDL "TriggerInfoSeq=AdServer::ChannelSvcs::ChannelUpdateBase_v28")

set_key_value(idlAndTheirNamespaces UserInfoManagerControllerIDL AdServer::UserInfoSvcs)
set_key_value(excludeClasses UserInfoManagerControllerIDL "UserInfoManagerSession NotReady ImplementationException UserInfoManagerController")
set_key_value(specialNamespaceForClass UserInfoManagerControllerIDL "GroupDescriptionSeq=AdServer::ChannelSvcs::Protected")


function(add_idl _target _idlfile target_dir)
  get_filename_component(IDL_FILE_NAME_WE ${_idlfile} NAME_WE)
#    set(MIDL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/Generated)
  set(MIDL_OUTPUT_PATH ${target_dir})
  set(MIDL_OUTPUT ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.ipp)
  set(OUTPUTC ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.cpp)
  set(OUTPUTHPP ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.hpp)
  set(OUTPUTS ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}_s.cpp)

  if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
      set(MIDL_ARCH win32)
  else()
      set(MIDL_ARCH x64)
  endif()

  file(MAKE_DIRECTORY ${target_dir})
  execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${target_dir})
  set (SRC ${CMAKE_CURRENT_LIST_DIR}/${_idlfile})
  set(FINDIDL_TARGET ${_target}_gen)
  add_custom_target(${FINDIDL_TARGET} DEPENDS "${OUTPUTS}")

  #get namespace for particular idl - if not found, return empty string and that mean that we do default build
  #otherwise we do build with particular script
  get_value(idlAndTheirNamespaces ${_target} namespace_idl)
  get_value(excludeClasses ${_target} exclude_classes)
  get_value(specialNamespaceForClass ${_target} special_namespace_for_class)

  if(namespace_idl)
      set(OUTPUTHPP_corbaTypes ${MIDL_OUTPUT_PATH}/CorbaTypes.hpp)
      set(OUTPUTC_corbaTypes ${MIDL_OUTPUT_PATH}/CorbaTypes.cpp)
      add_custom_command(
              OUTPUT ${MIDL_OUTPUT} ${OUTPUTC} ${OUTPUTS}
              COMMAND tao_idl ARGS  -Sp -in -ci .ipp -cs .cpp -hc .hpp -hs _s.hpp -ss _s.cpp  -I ${PROJECT_SOURCE_DIR} -I ${CORBA_INCLUDES}  ${SRC} -o ${MIDL_OUTPUT_PATH} 2> NUL
              #          COMMAND ${CMAKE_COMMAND} -E echo "done generating ${MIDL_OUTPUT_PATH}"
              #          COMMAND ${CMAKE_COMMAND} -E echo "OUTPUTC ${OUTPUTC};"
              #          COMMAND ${CMAKE_COMMAND} -E echo "OUTPUTHPP ${OUTPUTHPP};"

#              COMMAND ${CMAKE_COMMAND} -E echo "start sed"
              COMMAND sed -i "'s/if (0 == &_tao_elem)/if (true)/g'" ${OUTPUTC}
#              COMMAND ${CMAKE_COMMAND} -E echo "done sed"

#              COMMAND ${CMAKE_COMMAND} -E echo "start add_operator_sign.sh for ${OUTPUTHPP} with namespace ${namespace_idl} and exclude classes {${exclude_classes}} and special namespace for class {${special_namespace_for_class}}"
              COMMAND bash ${CMAKE_SOURCE_DIR}/cmake/utils/add_operator_sign.sh ${OUTPUTHPP} ${namespace_idl} ${exclude_classes} ${special_namespace_for_class}
#              COMMAND ${CMAKE_COMMAND} -E echo "done add_operator_sign.sh"

              COMMENT "Add IDL. ${_target}. With add_operator_sign.sh"
              DEPENDS ${SRC} ${FINDIDL_TARGET}
      )
  else()
      add_custom_command(
              OUTPUT ${MIDL_OUTPUT} ${OUTPUTC} ${OUTPUTS} ${OUTPUTHPP}
              COMMAND tao_idl ARGS  -Sp -in -ci .ipp -cs .cpp -hc .hpp -hs _s.hpp -ss _s.cpp  -I ${PROJECT_SOURCE_DIR} -I ${CORBA_INCLUDES}  ${SRC} -o ${MIDL_OUTPUT_PATH} 2> NUL
              #          COMMAND ${CMAKE_COMMAND} -E echo "done generating ${MIDL_OUTPUT_PATH}"
              #          COMMAND ${CMAKE_COMMAND} -E echo "OUTPUTC ${OUTPUTC};"
              #          COMMAND ${CMAKE_COMMAND} -E echo "OUTPUTHPP ${OUTPUTHPP};"
#              COMMAND ${CMAKE_COMMAND} -E echo "start sed"
              COMMAND sed -i "'s/if (0 == &_tao_elem)/if (true)/g'" ${OUTPUTC}
#              COMMAND ${CMAKE_COMMAND} -E echo "done sed"

              COMMENT "Add IDL. ${_target}."
              DEPENDS ${SRC} ${FINDIDL_TARGET}
      )
  endif()

  #only for CorbaTypes

  cmake_parse_arguments(FINDIDL "" "TLBIMP" "" ${ARGN})

  if(FINDIDL_TLBIMP)
      file(GLOB TLBIMPv7_FILES "C:/Program Files*/Microsoft SDKs/Windows/v7*/bin/TlbImp.exe")
      file(GLOB TLBIMPv8_FILES "C:/Program Files*/Microsoft SDKs/Windows/v8*/bin/*/TlbImp.exe")
      file(GLOB TLBIMPv10_FILES "C:/Program Files*/Microsoft SDKs/Windows/v10*/bin/*/TlbImp.exe")

      list(APPEND TLBIMP_FILES ${TLBIMPv7_FILES} ${TLBIMPv8_FILES} ${TLBIMPv10_FILES})

      if(TLBIMP_FILES)
          list(GET TLBIMP_FILES -1 TLBIMP_FILE)
      endif()

      if (NOT TLBIMP_FILE)
          message(FATAL_ERROR "Cannot found tlbimp.exe. Try to download .NET Framework SDK and .NET Framework targeting pack.")
          return()
      endif()

      message(STATUS "Found tlbimp.exe: " ${TLBIMP_FILE})

      set(TLBIMP_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

      if("${TLBIMP_OUTPUT_PATH}" STREQUAL "")
          set(TLBIMP_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
      endif()

      set(TLBIMP_OUTPUT ${TLBIMP_OUTPUT_PATH}/${FINDIDL_TLBIMP}.dll)

      add_custom_command(
          OUTPUT  ${TLBIMP_OUTPUT}
          COMMAND ${TLBIMP_FILE} "${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.tlb" "/out:${TLBIMP_OUTPUT}" ${TLBIMP_FLAGS}
          DEPENDS ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.tlb
          VERBATIM
          )

      add_custom_target(${FINDIDL_TARGET} DEPENDS ${MIDL_OUTPUT} ${TLBIMP_OUTPUT} SOURCES ${_idlfile})

      add_library(${FINDIDL_TLBIMP} SHARED IMPORTED GLOBAL)
      add_dependencies(${FINDIDL_TLBIMP} ${FINDIDL_TARGET})

      set_target_properties(${FINDIDL_TLBIMP}
          PROPERTIES
          IMPORTED_LOCATION "${TLBIMP_OUTPUT}"
          IMPORTED_COMMON_LANGUAGE_RUNTIME "CSharp"
          )
  else()
  endif()
  add_library(${_target} SHARED
      ${OUTPUTC} ${OUTPUTS}
  )

  target_link_libraries(
      ${_target}
      ACE
      TAO TAO_AnyTypeCode  TAO_CodecFactory TAO_CosEvent TAO_CosNaming TAO_CosNotification TAO_DynamicAny TAO_EndpointPolicy TAO_FaultTolerance
      TAO_FT_ClientORB TAO_FT_ServerORB TAO_FTORB_Utils TAO_IORManip TAO_IORTable
      TAO_Messaging TAO_PI TAO_PI_Server TAO_PortableGroup TAO_PortableServer TAO_Security TAO_SSLIOP TAO_TC TAO_TC_IIOP TAO_Valuetype ACE_SSL
  )

  install(TARGETS ${_target} DESTINATION ${INSTALL_DIR})

  add_dependencies(${_target} ${FINDIDL_TARGET})
  target_include_directories(${_target} INTERFACE ${MIDL_OUTPUT_PATH})

endfunction()
