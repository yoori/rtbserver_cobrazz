ADD_SUBDIRECTORY(Compatibility)

set(USER_INFO_MANAGER_GRPC_PROTO_DIR ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager)
set(USER_INFO_MANAGER_GRPC_PROTO UserInfoManagerGrpc.proto)

add_grpc_sources(
  UserInfoManagerGrpc
  ${USER_INFO_MANAGER_GRPC_PROTO}
  ${USER_INFO_MANAGER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(UserInfoManagerGrpc
  PUBLIC
    protobuf::libprotobuf
    GrpcBatch
)

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
  gRPC::grpc++
  PRIVATE
  Sync)

add_library(UserInfoContainer SHARED
  UserInfoContainer.cpp
  UserOperationLoader.cpp
  UserOperationSaver.cpp

)
target_link_libraries(UserInfoContainer
  PRIVATE
  Generics
  PrivacyFilter
  Commons
  Allocator
  ProfilingCommons
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
  UserInfoManagerLogger.cpp

)
target_link_libraries(UserInfoManager
  PRIVATE
    jemalloc

    TAO::PortableServer
    TAO::Valuetype

    AdServerCommonsXSD
    Allocator
    CampaignCommonsIDL
    CampaignServerIDL
    Commons
    CorbaTypesIDL
    CORBACommons
    CORBAConfig
    Generics
    GrpcBatch
    HttpServer
    LogCommons
    Logger
    PrivacyFilter
    ProcessControlIDL
    ProfilingCommons
    ProfileMap
    ProfileMapRocksDB
    Sync

    UserInfoManagerConfigXSD
    UserInfoManagerGrpc

    UserInfoContainer
    gRPC::grpc++

)

target_include_directories(UserInfoManager PRIVATE
  ${Protobuf_INCLUDE_DIRS}
)

if(TARGET Protobuf::libprotobuf)
  target_link_libraries(UserInfoManager PRIVATE Protobuf::libprotobuf)
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(UserInfoManager PRIVATE protobuf::libprotobuf)
else()
  target_link_libraries(UserInfoManager PRIVATE ${Protobuf_LIBRARIES})
endif()

#install(TARGETS UserInfoManager UserInfoContainer DESTINATION ${INSTALL_DIR})
