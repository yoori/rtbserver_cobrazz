add_executable(UserInfoAdmin2
  Application.cpp
)

target_link_libraries(UserInfoAdmin2
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

target_include_directories(UserInfoAdmin2 PRIVATE
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoController2
)
