set(URL_INFO_STORAGE_PROTO_DIR ${PB_DIRECTORY}/RequestInfoSvcs/UrlInfoStorage)
add_grpc_sources(UrlInfoStorageGrpcProto UrlInfoStorage.proto ${URL_INFO_STORAGE_PROTO_DIR})
target_link_libraries(UrlInfoStorageGrpcProto PUBLIC protobuf::libprotobuf GrpcBatch)

add_library(UrlInfoStorageGrpcClient STATIC)
add_adserver_grpc_client_sources(UrlInfoStorageGrpcClient
  ${CMAKE_CURRENT_SOURCE_DIR}/UrlInfoStorage.proto ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::RequestInfoSvcs)
target_link_libraries(UrlInfoStorageGrpcClient
  PUBLIC Commons UrlInfoStorageGrpcProto GrpcBatch gRPC::grpc++)

add_library(UrlInfoStorageImpl STATIC
  UrlInfoStorage.cpp UrlInfoStorageGrpc.cpp UrlInfoStorageHttp.cpp)
target_link_libraries(UrlInfoStorageImpl
  PUBLIC UrlInfoStorageGrpcProto ProfileMapRocksDB Commons HttpServer Generics Logger)

add_executable(UrlInfoStorage UrlInfoStorageMain.cpp)
target_link_libraries(UrlInfoStorage PRIVATE UrlInfoStorageImpl UrlInfoStorageConfigXSD)
