add_library(CookieManager SHARED
  CookieManager.cpp
)

target_link_libraries(CookieManager
  PRIVATE
  HTTP
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
  PRIVATE
  Commons
  FeConfigXSD
  Boost::headers
)

add_library(ProcessingTemplates SHARED
  ProcessingTemplates.cpp
)

target_link_libraries(ProcessingTemplates
  PRIVATE
  Generics
  FCGI
)

add_library(RequestMatchers SHARED
  RequestMatchers.cpp
  UserAgentMatcher.cpp
)

target_link_libraries(RequestMatchers
  PRIVATE
    BLogic
    Generics
    HTTP
)

add_library(UserIdController SHARED
  UserIdController.cpp
)

target_link_libraries(UserIdController
  PRIVATE
  Generics
  Commons
)
