add_library(UserInfoManagerDump STATIC
  UserInfoManagerDump.cpp
)

target_include_directories(UserInfoManagerDump
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(UserInfoManagerProxy
  Application.cpp
)

target_link_libraries(UserInfoManagerProxy
  UserInfoManagerDump
  UserInfoManagerGrpc
  GrpcBatch
  Commons
  Generics
  gRPC::grpc++
  protobuf::libprotobuf
)

#install(TARGETS UserInfoManagerProxy DESTINATION ${INSTALL_DIR})
