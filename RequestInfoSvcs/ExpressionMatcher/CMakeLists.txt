ADD_SUBDIRECTORY(Compatibility)

set(EXPRESSION_MATCHER_GRPC_PROTO_DIR ${PB_DIRECTORY}/RequestInfoSvcs/ExpressionMatcher)
set(EXPRESSION_MATCHER_GRPC_PROTO ExpressionMatcherGrpc.proto)

add_grpc_sources(
  ExpressionMatcherGrpcProto
  ${EXPRESSION_MATCHER_GRPC_PROTO}
  ${EXPRESSION_MATCHER_GRPC_PROTO_DIR}
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(ExpressionMatcherGrpcProto protobuf::libprotobuf GrpcBatch)

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
  gRPC::grpc++)

add_library(ExpressionChannelMatcher STATIC
  ChannelMatcher.cpp
)

add_library(UserTriggerMatchContainer STATIC
  UserTriggerMatchContainer.cpp
)

target_link_libraries(UserTriggerMatchContainer
  UserTriggerMatchProfilePST
  RequestTriggerMatchProfilePST
)

add_library(UserInventoryContainer STATIC
  UserInventoryContainer.cpp
)

add_dependencies(UserInventoryContainer UserChannelInventoryProfilePST)

add_library(UserColoReachContainer STATIC
  UserColoReachContainer.cpp
)

add_dependencies(UserColoReachContainer UserChannelInventoryProfilePST)

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
  UserColoReachContainer
  UserInventoryContainer
  UserTriggerMatchContainer
  ExpressionChannelMatcher
  ExpressionChannelIndex

  ExpressionMatcherGrpcClient
  ExpressionMatcherGrpcProto

  CampaignServerIDL
  CORBACommons

  ConsiderMessagesPST
  RequestOperationProfilePST
  UserChannelInventoryProfilePST
  RequestTriggerMatchProfilePST

  ExpressionMatcherCompatibility
  UserInfoClient

  AdServerCommonsXSD
  ExpressionMatcherConfigXSD

  LogCommons
  Commons
  ProfilingCommons
  SNMPAgent
  ProfileMap
  ProfileMapRocksDB
  XMLUtility
  CampaignTypes
  CampaignConfigSource

  Logger
  String
  Generics
  Sync
  pthread
)
