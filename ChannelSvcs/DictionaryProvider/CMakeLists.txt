add_grpc_sources(DictionaryProviderGrpc DictionaryProvider.proto
  ${PB_DIRECTORY}/ChannelSvcs/DictionaryProvider)
target_sources(DictionaryProviderGrpc PRIVATE
  ../DictionaryProviderClient/DictionaryProviderClient.cpp)
target_link_libraries(DictionaryProviderGrpc PUBLIC gRPC::grpc++ protobuf::libprotobuf)

add_library(DictionaryProviderImpl SHARED DictionaryProviderImpl.cpp DictionaryContainer.cpp)
target_link_libraries(DictionaryProviderImpl
  PUBLIC DictionaryProviderGrpc DictionaryProviderConfigXSD Generics Logger Commons Sync)

add_executable(DictionaryProvider DictionaryProviderMain.cpp)
target_link_libraries(DictionaryProvider PRIVATE DictionaryProviderImpl)
