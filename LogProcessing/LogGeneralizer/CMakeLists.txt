add_grpc_sources(LogGeneralizerGrpc LogGeneralizer.proto
  ${PB_DIRECTORY}/LogProcessing/LogGeneralizer)
target_sources(LogGeneralizerGrpc PRIVATE LogGeneralizerGrpc.cpp)
target_link_libraries(LogGeneralizerGrpc PUBLIC gRPC::grpc++ protobuf::libprotobuf Generics)

add_library(LogGeneralizerLib STATIC
  LogGeneralizerImplInit.cpp
  LogGeneralizerImplInit2.cpp
  LogGeneralizerImpl.cpp
  LogDbSaverImpl.cpp
  DbConnectionFactory.cpp
  ProcessingContexts.cpp
  SiteReferrerStatCsvSaver.cpp
)

target_link_libraries(LogGeneralizerLib
  PUBLIC
    LogGeneralizerGrpc
    LogCommons
  PRIVATE
    Postgres
    Commons
    Generics
    HTTP
    Logger
    Sync
)

add_executable(LogGeneralizer
  LogGeneralizerMain.cpp
  LogGeneralizerHttp.cpp
)

target_link_libraries(LogGeneralizer
  PRIVATE
    LogGeneralizerLib
    Threads::Threads
    Generics
    Commons
    Logger
    HttpServer
    GrpcBatch
    LogGeneralizerConfigXSD
    SNMPAgent
)
