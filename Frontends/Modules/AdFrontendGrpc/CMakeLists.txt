add_library(AdFrontendGrpc SHARED
  AdFrontend.cpp
  DebugSink.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(AdFrontendGrpc
  CORBACommons
  CommonModule
  Commons
  CookieManager
  ExpressionChannelIndex
  FeConfigXSD
  FrontendStatistic
  Geoip
  HTTP
  Polyglot
  ProcessingTemplates
  RequestMatchers
  SecToken
  Stream
)
