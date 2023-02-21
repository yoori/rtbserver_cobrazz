add_library(CookieManager SHARED
CookieManager.cpp
)
target_link_libraries(CookieManager

 HTTP
 Commons
 CORBACommons
 FeConfigXSD

)

add_library(FCGI SHARED
FCGI.cpp 
  FrontendInterface.cpp 
  FrontendTaskPool.cpp
)

target_link_libraries(FCGI
 Commons
 Stream
 FeConfigXSD

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
 Generics
 FCGI
 Commons
)


add_library(RequestMatchers SHARED

RequestMatchers.cpp 
  UserInfoClient.cpp 
  UserBindClient.cpp 
  UserAgentMatcher.cpp 
  ChannelServerSessionPool.cpp
)

target_link_libraries(RequestMatchers
 HTTP
 BLogic
 Commons
 CORBACommons
 UserInfoClusterControlIDL UserInfoManagerControllerIDL UserInfoManagerControllerL
UserBindControllerIDL  UserBindControllerStubs
 ChannelSessionFactory
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
#install(TARGETS UserIdController RequestMatchers ProcessingTemplates FrontendStatistic  FCGI CookieManager  DESTINATION ${INSTALL_DIR})