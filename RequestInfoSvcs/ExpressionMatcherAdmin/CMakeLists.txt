
add_executable(ExpressionMatcherAdmin
  Application.cpp
)
target_link_libraries(ExpressionMatcherAdmin
  String
  Generics
  UtilCommons

  ExpressionMatcherGrpcProto
  gRPC::grpc++
  pthread

)



#install(TARGETS ExpressionMatcherAdmin  DESTINATION ${INSTALL_DIR})
