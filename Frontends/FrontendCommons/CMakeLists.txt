add_library(CookieManager SHARED
  CookieManager.cpp
)

target_link_libraries(CookieManager
  HTTP
  Commons
  FeConfigXSD
)

add_library(FCGI SHARED
  HttpResponse.cpp
  HttpRequest.cpp
  FrontendInterface.cpp
  RequestTask.cpp
  FrontendWorkers.cpp
)

target_link_libraries(FCGI
  Commons
  Stream
  FeConfigXSD
)

add_library(ProcessingTemplates SHARED
  ProcessingTemplates.cpp
)

target_link_libraries(ProcessingTemplates
  Generics
  FCGI
  Commons
)

add_library(RequestMatchers SHARED
  RequestMatchers.cpp
  UserAgentMatcher.cpp
)

target_link_libraries(RequestMatchers
  HTTP
  BLogic
  Commons

  UserInfoClient
  UserBindControllerGrpc
  UserBindServerGrpc
  UserBindServerGrpcClient
  UserBindClient
  GrpcBatch
  gRPC::grpc++
  ChannelClient

  FeConfigXSD

  pcre
)

add_library(UserIdController SHARED
  UserIdController.cpp
)

target_link_libraries(UserIdController
  Generics
  Commons
)
