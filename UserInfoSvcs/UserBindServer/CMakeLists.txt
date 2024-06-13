add_idl(UserBindServerIDL UserBindServer.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserBindServer)

target_link_libraries(UserBindServerIDL
  CORBACommons
  Commons
  CorbaTypesIDL
  UserBindOperationProfilePST
)

add_library(UserBindContainer STATIC
  ExternalIdHashAdapter.cpp 
  ChunkUtils.cpp 
  UserBindContainer.cpp 
  UserBindChunk.cpp
  UserBindChunkTypes.cpp
  UserBindChunkTwoLayers.cpp
  BindRequestContainer.cpp 
  BindRequestChunk.cpp
)

target_link_libraries(UserBindContainer
  Commons
  Generics
  LogCommons
  PrivacyFilter
  ProfileMap
  UServerUtils
)

add_pst(UserBindOperationProfilePST UserBindOperationProfile.pst ${PST_DIRECTORY}/UserInfoSvcs/UserBindServer)

add_executable(UserBindServer
  GrpcService.cpp
  UserBindServerMain.cpp
  UserBindServerImpl.cpp
  UserBindOperationSaver.cpp
  UserBindOperationLoader.cpp
)

message(STATUS "Use GrpcTargets.cmake: ${UNIXCOMMONS}/src/UServerUtils/Grpc/Generator/GrpcTargets.cmake")
include(${UNIXCOMMONS}/src/UServerUtils/Grpc/Generator/GrpcTargets.cmake)

add_grpc_library(UserBindServerProto
  PROTOS
    UserBindServer.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(UserBindServer
  CORBACommons
  Commons
  LogCommons
  MessageSaver
  PrivacyFilter
  ProcessControlVars
  ProfileMap
  UserInfoStatistics
  UServerUtils
  UserBindContainer
  UserBindServerConfigXSD
  UserBindServerIDL
  UserBindServerProto
  UServerUtils
)
