add_executable(UserBindAdmin
  Application.cpp
)

add_executable(UserBindAdmin2
  UserBindAdmin2.cpp
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

target_link_libraries(UserBindAdmin2
  gRPC::grpc++

  Generics
  Commons
  Logger
  UserBindClient
  UserBindServerGrpcClient
  UserBindControllerGrpc
)

target_include_directories(UserBindAdmin2 PRIVATE
  ${PB_DIRECTORY}/UserInfoSvcs/UserBindServer
  ${PB_DIRECTORY}/UserInfoSvcs/UserBindController2
)
