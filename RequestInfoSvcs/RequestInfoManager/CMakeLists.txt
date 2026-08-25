ADD_SUBDIRECTORY(Compatibility)

set(REQUEST_INFO_MANAGER_GRPC_PROTO_DIR ${PB_DIRECTORY}/RequestInfoSvcs/RequestInfoManager)
set(REQUEST_INFO_MANAGER_GRPC_PROTO RequestInfoManagerGrpc.proto)

add_grpc_sources(
  RequestInfoManagerGrpcProto
  ${REQUEST_INFO_MANAGER_GRPC_PROTO}
  ${REQUEST_INFO_MANAGER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(RequestInfoManagerGrpcProto
  PUBLIC
    protobuf::libprotobuf
    GrpcBatch
)

add_library(RequestInfoManagerGrpcClient STATIC)

add_adserver_grpc_client_sources(
  RequestInfoManagerGrpcClient
  ${CMAKE_SOURCE_DIR}/RequestInfoSvcs/RequestInfoManager/RequestInfoManagerGrpc.proto
  ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::RequestInfoSvcs
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(RequestInfoManagerGrpcClient
  PUBLIC
    Commons
    RequestInfoManagerGrpcProto
    GrpcBatch
    gRPC::grpc++
  PRIVATE
    Sync
)

add_library(BillingProcessor SHARED
  BillingProcessor.cpp
)

target_link_libraries(BillingProcessor
  PRIVATE
    Generics
    Commons
    BillingServerGrpcClient
)

add_library(PassbackContainer SHARED
  PassbackContainer.cpp
)

target_link_libraries(PassbackContainer
  PRIVATE
    Generics
    Commons
    PassbackProfilePST
    ProfileMapRocksDB
)

add_library(RequestInfoContainer SHARED
  RequestInfoContainer.cpp
)

target_link_libraries(RequestInfoContainer
  PRIVATE
    Generics
    Commons
    CampaignBilling
    ProfileMapRocksDB
    RequestProfilePST
    RequestInfoManagerCompatibility
)

add_executable(RequestInfoManager
  RequestInfoManagerGrpc.cpp
  RequestInfoManagerMain.cpp
  RequestInfoManagerImpl.cpp
  RequestLogLoader.cpp
  RequestOperationDistributor.cpp
  ExpressionMatcherNotifier.cpp
)

target_link_libraries(RequestInfoManager
  PRIVATE
    jemalloc
    TAO::PortableServer
    TAO::Valuetype
    CampaignCommonsIDL
    CorbaTypesIDL
    CORBACommons
    Generics
    GrpcBatch
    ProcessControlIDL
    SNMPAgent
    Sync
    RequestInfoManagerGrpcProto
    Commons
    HTTP
    Logger
    CORBAConfig
    RequestInfoManagerConfigXSD
    BillingProcessor
    PassbackContainer
    RequestInfoContainer
    UserInfoClient
    CampaignServerIDL
    LogCommons
    RequestCompatibility
    MessageSaver
    ProfileMapRocksDB
    RequestLogLoader
    RequestOperationSaver
    RequestOutLogger
    TagRequestProfiler
    UserActionInfoContainer
    UserCampaignReachContainer
    UserFraudProtectionContainer
    UserSiteReachContainer
    UserTagRequestMergeContainer
    gRPC::grpc++
)

if(TARGET Protobuf::libprotobuf)
  target_link_libraries(RequestInfoManager
    PRIVATE
      Protobuf::libprotobuf
  )
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(RequestInfoManager
    PRIVATE
      protobuf::libprotobuf
  )
else()
  target_link_libraries(RequestInfoManager
    PRIVATE
      ${Protobuf_LIBRARIES}
  )
endif()

add_library(RequestLogLoader SHARED
  RequestLogLoader.cpp
  RequestOperationLoader.cpp
  RequestInfoManagerStats.cpp
)

target_link_libraries(RequestLogLoader
  PRIVATE
    Generics
    LogCommons
    ProfilingCommons
    RequestOperationProfilePST
    RequestInfoManagerCompatibility
    RequestCompatibility
)

add_library(RequestOperationSaver SHARED
  RequestOperationSaver.cpp
)

target_link_libraries(RequestOperationSaver
  PRIVATE
    Generics
    MessageSaver
    RequestOperationProfilePST
)

add_library(RequestOutLogger STATIC
  RequestOutLogger.cpp
)

target_link_libraries(RequestOutLogger
  PRIVATE
    LogCommons
)

add_library(TagRequestProfiler STATIC
  TagRequestProfiler.cpp
)

target_link_libraries(TagRequestProfiler
  PRIVATE
    LogCommons
    zmq
)

add_library(UserActionInfoContainer STATIC
  UserActionInfoContainer.cpp
)

target_link_libraries(UserActionInfoContainer
  PRIVATE
    UserActionProfilePST
    LogCommons
    ProfileMapRocksDB
)

add_library(UserCampaignReachContainer STATIC
  UserCampaignReachContainer.cpp
)

target_link_libraries(UserCampaignReachContainer
  PRIVATE
    UserCampaignReachProfilePST
    LogCommons
    ProfileMapRocksDB
)

add_library(UserFraudProtectionContainer STATIC
  UserFraudProtectionContainer.cpp
)

target_link_libraries(UserFraudProtectionContainer
  PRIVATE
    UserFraudProtectionProfilePST
    LogCommons
    ProfileMapRocksDB
)

add_library(UserSiteReachContainer STATIC
  UserSiteReachContainer.cpp
)

target_link_libraries(UserSiteReachContainer
  PRIVATE
    UserSiteReachProfilePST
    LogCommons
    ProfileMapRocksDB
)

add_library(UserTagRequestMergeContainer STATIC
  UserTagRequestMergeContainer.cpp
)

target_link_libraries(UserTagRequestMergeContainer
  PRIVATE
    UserTagRequestMergeProfilePST
    LogCommons
    ProfileMapRocksDB
)
