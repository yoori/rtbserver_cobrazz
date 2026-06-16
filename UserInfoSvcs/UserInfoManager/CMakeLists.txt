ADD_SUBDIRECTORY(Compatibility)

set(USER_INFO_MANAGER_GRPC_PROTO_DIR ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager)
set(USER_INFO_MANAGER_GRPC_PROTO UserInfoManagerGrpc.proto)

add_grpc_sources(
  UserInfoManagerGrpc
  ${USER_INFO_MANAGER_GRPC_PROTO}
  ${USER_INFO_MANAGER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(UserInfoManagerGrpc protobuf::libprotobuf GrpcBatch)

add_library(UserInfoManagerGrpcClient STATIC)

add_adserver_grpc_client_sources(
  UserInfoManagerGrpcClient
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserInfoManager/UserInfoManagerGrpc.proto
  ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::UserInfoSvcs
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(UserInfoManagerGrpcClient
  PUBLIC
  Commons
  GrpcBatch
  UserInfoManagerGrpc
  gRPC::grpc++)

add_library(UserInfoContainer SHARED
  UserInfoContainer.cpp
  UserOperationLoader.cpp
  UserOperationSaver.cpp

)
add_dependencies(UserInfoContainer UserChannelBaseProfilePST )
target_link_libraries(UserInfoContainer
  Generics
  PrivacyFilter
  Commons
  Allocator
  PlainStorage2
  PlainStorage3
  ProfileMap
  ProfileMapRocksDB

  ChannelMatcher
  LogCommons
  UserFreqCapProfile
  UserOperationProfilesPST
  UserBaseProfileAdapter
  UserHistoryProfileAdapter
  UserOperationProfilesAdapter
  UserFreqCapProfileAdapter

)


add_executable(UserInfoManager
  UserInfoManagerMain.cpp
  FileRWStats.cpp
  UserInfoConfigCampaignServerSource.cpp
  UserInfoManagerCore.cpp
  UserInfoManagerGrpc.cpp
  UserInfoContainer.cpp
  UserInfoManagerLogger.cpp

)
target_link_libraries(UserInfoManager
  tcmalloc
  CORBACommons
  LogCommons
  HttpServer
  Commons
  AdServerCommonsXSD
  PlainStorage2
  PrivacyFilter
  ProfileMap
  ProfileMapRocksDB

  UserInfoManagerConfigXSD
  UserInfoManagerGrpc
  CampaignServerIDL

  UserInfoContainer
  UserOperationProfilesPST

)

target_include_directories(UserInfoManager PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)

if(TARGET Protobuf::libprotobuf)
  target_link_libraries(UserInfoManager Protobuf::libprotobuf)
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(UserInfoManager protobuf::libprotobuf)
else()
  target_link_libraries(UserInfoManager ${Protobuf_LIBRARIES})
endif()

target_link_libraries(UserInfoManager gRPC::grpc++)

#install(TARGETS UserInfoManager UserInfoContainer DESTINATION ${INSTALL_DIR})
