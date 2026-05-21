add_executable(UserInfoAdmin
  Application.cpp
)

target_link_libraries(UserInfoAdmin
  Generics
  CORBACommons
  UtilCommons
  ChannelMatcher
  UserFreqCapProfile
  UserInfoClient
  UserInfoManagerGrpc
  UserInfoManagerGrpcClient
  UserInfoControllerGrpc
  gRPC::grpc++
)

target_include_directories(UserInfoAdmin PRIVATE
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoController
)
