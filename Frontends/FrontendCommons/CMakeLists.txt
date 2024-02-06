add_library(CookieManager SHARED
  CookieManager.cpp
)

target_link_libraries(CookieManager
  CORBACommons
  Commons
  FeConfigXSD
  HTTP
)

add_library(FrontendsPool SHARED
  FrontendsPool.cpp
)

target_link_libraries(FrontendsPool
  Commons
  UServerUtils
)

add_library(FCGI SHARED
  FCGI.cpp
  FrontendInterface.cpp
  FrontendTaskPool.cpp
  HttpRequest.cpp
  HttpResponse.cpp
)

target_link_libraries(FCGI
  Commons
  FeConfigXSD
  FrontendsPool
  Stream
  UServerUtils
)

add_library(FrontendStatistic SHARED
  FrontendStatistic.cpp
)

target_link_libraries(FrontendStatistic
 CORBACommons
 Logger
 StatsDumper
)

add_library(ProcessingTemplates SHARED
  ProcessingTemplates.cpp
)

target_link_libraries(ProcessingTemplates
  Commons
  FCGI
  Generics
)

add_library(RequestMatchers SHARED
  ChannelServerSessionPool.cpp
  RequestMatchers.cpp
  UserAgentMatcher.cpp
  UserBindClient.cpp
  UserInfoClient.cpp
)

target_link_libraries(RequestMatchers
  BLogic
  CORBACommons
  ChannelSessionFactory
  Commons
  FeConfigXSD
  HTTP
  UserBindControllerIDL
  UserBindControllerStubs
  UserInfoClusterControlIDL
  UserInfoManagerControllerIDL
  UserInfoManagerControllerL
  UServerUtils
  pcre
)

add_library(UserIdController SHARED
  UserIdController.cpp
)

target_link_libraries(UserIdController
  Commons
  Generics
)