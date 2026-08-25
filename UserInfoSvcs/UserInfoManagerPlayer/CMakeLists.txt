add_executable(UserInfoManagerPlayer
  Application.cpp
)

target_link_libraries(UserInfoManagerPlayer
  PRIVATE
    UserInfoManagerDump
    UserInfoManagerGrpc
    Generics
    gRPC::grpc++
)

#install(TARGETS UserInfoManagerPlayer DESTINATION ${INSTALL_DIR})
