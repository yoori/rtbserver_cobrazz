add_idl(ChannelCommonsIDL ChannelCommons.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)
add_idl(ChannelServerIDL ChannelServer.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)
add_idl(ChannelUpdateBaseIDL ChannelUpdateBase.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelCommons)

target_link_libraries(ChannelCommonsIDL
  BLogic
  CORBACommons
  Commons
  CorbaTypesIDL
  LogCommons
)

target_link_libraries(ChannelUpdateBaseIDL
  BLogic
  CORBACommons
  ChannelCommonsIDL
  Commons
  CorbaTypesIDL
  LogCommons
)

target_link_libraries(ChannelServerIDL
  Commons
  LogCommons
  CORBACommons
  CorbaTypesIDL
  ChannelCommonsIDL
  ChannelUpdateBaseIDL
  CampaignServerIDL
  CorbaObjectRefIDL
  ProcessControlIDL
)

include(${UNIXCOMMONS}/src/UServerUtils/Grpc/Generator/GrpcTargets.cmake)
add_grpc_library(ChannelServerProto
  PROTOS
    ChannelServer.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ChannelCommons SHARED
  ChannelUtils.cpp ChannelUpdateStatLogger.cpp TriggerParser.cpp
)

target_link_libraries(ChannelCommons
  BLogic
  ChannelCommonsIDL
  ChannelServerProto
  Commons
  Generics
  HTTP
  LogCommons
  UServerUtils
)