add_executable(UserInfoAdmin
  Application.cpp
)

target_link_libraries(UserInfoAdmin
  PRIVATE
    ChannelMatcher
    Commons
    CORBACommons
    Generics
    GrpcBatch
    Logger
    ProcessControlIDL
    UserFreqCapProfile
    UserInfoClient
    UserInfoManagerGrpc
    UserInfoManagerGrpcClient
    UserInfoControllerGrpc
    gRPC::grpc++
)

if(TARGET Protobuf::libprotobuf)
  target_link_libraries(UserInfoAdmin
    PRIVATE
      Protobuf::libprotobuf
  )
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(UserInfoAdmin
    PRIVATE
      protobuf::libprotobuf
  )
else()
  target_link_libraries(UserInfoAdmin
    PRIVATE
      ${Protobuf_LIBRARIES}
  )
endif()

target_include_directories(UserInfoAdmin PRIVATE
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoController
)
