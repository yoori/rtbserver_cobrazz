add_executable(UserBindAdmin
  Application.cpp
)

target_link_libraries(UserBindAdmin
  gRPC::grpc++

  Generics
  CORBACommons

  Commons

  UserBindControllerStubs
  UserBindServerIDL
  UserBindControllerIDL
  UserBindServerGrpc
)
