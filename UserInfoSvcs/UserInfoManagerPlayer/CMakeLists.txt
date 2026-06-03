add_executable(UserInfoManagerPlayer
  Application.cpp
)

target_link_libraries(UserInfoManagerPlayer
  UserInfoManagerDump
  UserInfoManagerGrpc
  GrpcBatch
  Commons
  Generics
  gRPC::grpc++
  protobuf::libprotobuf
)

#install(TARGETS UserInfoManagerPlayer DESTINATION ${INSTALL_DIR})
