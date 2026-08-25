ADD_SUBDIRECTORY(Compatibility)

set(EXPRESSION_MATCHER_GRPC_PROTO_DIR ${PB_DIRECTORY}/RequestInfoSvcs/ExpressionMatcher)
set(EXPRESSION_MATCHER_GRPC_PROTO ExpressionMatcherGrpc.proto)

add_grpc_sources(
  ExpressionMatcherGrpcProto
  ${EXPRESSION_MATCHER_GRPC_PROTO}
  ${EXPRESSION_MATCHER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(ExpressionMatcherGrpcProto
  PUBLIC
    protobuf::libprotobuf
    GrpcBatch
)

add_library(ExpressionMatcherGrpcClient STATIC)

add_adserver_grpc_client_sources(
  ExpressionMatcherGrpcClient
  ${CMAKE_SOURCE_DIR}/RequestInfoSvcs/ExpressionMatcher/ExpressionMatcherGrpc.proto
  ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::RequestInfoSvcs
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(ExpressionMatcherGrpcClient
  PUBLIC
    Commons
    ExpressionMatcherGrpcProto
    GrpcBatch
    gRPC::grpc++
  PRIVATE
    Sync
)

add_library(ExpressionChannelMatcher STATIC
  ChannelMatcher.cpp
)

target_link_libraries(ExpressionChannelMatcher
  PRIVATE
    ExpressionChannelIndex
    Generics
    Logger
    Sync
)

add_library(UserTriggerMatchContainer STATIC
  UserTriggerMatchContainer.cpp
)

target_link_libraries(UserTriggerMatchContainer
  PRIVATE
    UserTriggerMatchProfilePST
    RequestTriggerMatchProfilePST
    Commons
    Generics
    Logger
    ProfileMap
    ProfileMapRocksDB
    ProfilingCommons
)

add_library(UserInventoryContainer STATIC
  UserInventoryContainer.cpp
)

target_link_libraries(UserInventoryContainer
  PRIVATE
    UserChannelInventoryProfilePST
    CampaignTypes
    Commons
    Generics
    Logger
    ProfileMap
    ProfileMapRocksDB
    ProfilingCommons
)

add_library(UserColoReachContainer STATIC
  UserColoReachContainer.cpp
)

target_link_libraries(UserColoReachContainer
  PRIVATE
    UserChannelInventoryProfilePST
    Commons
    Generics
    Logger
    ProfileMap
    ProfileMapRocksDB
    ProfilingCommons
)

add_executable(ExpressionMatcher
  ExpressionMatcherGrpc.cpp
  ExpressionMatcherLogLoader.cpp
  ExpressionMatcherImpl.cpp
  ExpressionMatcherMain.cpp
  ExpressionMatcherOutLogger.cpp
  ExpressionMatcherStats.cpp
  UserTriggerMatchProfileProviderImpl.cpp
  ConversionProcessor.cpp
)

target_link_libraries(ExpressionMatcher
  PRIVATE
    jemalloc
    TAO::PortableServer
    TAO::Valuetype
    UserColoReachContainer
    UserInventoryContainer
    UserTriggerMatchContainer
    ExpressionChannelMatcher
    ExpressionChannel
    ExpressionMatcherGrpcClient
    ExpressionMatcherGrpcProto
    CampaignCommonsIDL
    CampaignServerIDL
    CorbaTypesIDL
    CORBACommons
    ProcessControlIDL
    ConsiderMessagesPST
    RequestOperationProfilePST
    UserInfoClient
    AdServerCommonsXSD
    ExpressionMatcherConfigXSD
    LogCommons
    Commons
    CORBAConfig
    HTTP
    ProfilingCommons
    SNMPAgent
    ProfileMap
    ProfileMapRocksDB
    XMLUtility
    Logger
    Generics
    Sync
)
