add_idl(UserBindServerIDL UserBindServer.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserBindServer)

target_link_libraries(UserBindServerIDL
  Commons
  CORBACommons
  CorbaTypesIDL
  UserBindOperationProfilePST
)

add_library(UserBindContainer STATIC
  ExternalIdHashAdapter.cpp 
  ChunkUtils.cpp 
  UserBindContainer.cpp 
  UserBindChunk.cpp 
  BindRequestContainer.cpp 
  BindRequestChunk.cpp
)

target_link_libraries(UserBindContainer
  Generics
  PrivacyFilter
  Commons
  LogCommons
  ProfileMap
)

add_pst(UserBindOperationProfilePST UserBindOperationProfile.pst ${PST_DIRECTORY}/UserInfoSvcs/UserBindServer)

add_executable(UserBindServer
  GrpcService.cpp
  UserBindServerMain.cpp
  UserBindServerImpl.cpp
  UserBindOperationSaver.cpp
  UserBindOperationLoader.cpp
)

include(${UNIXCOMMONS}/src/UServerUtils/Grpc/Generator/GrpcTargets.cmake)
add_grpc_library(UserBindServerProto
  PROTOS
    UserBindServer.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(UserBindServer
  CORBACommons
  PrivacyFilter
  ProcessControlVars
  Commons
  ProfileMap
  MessageSaver
  LogCommons
  UserBindContainer
  UserBindServerConfigXSD
  UserBindServerIDL
  UserBindServerProto
  UServerUtils
)