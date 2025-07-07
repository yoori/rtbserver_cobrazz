add_library(DirectoryModule SHARED
  DirectoryModule.cpp
)

target_link_libraries(DirectoryModule
  Stream
  Geoip
  CORBACommons
  Commons
  DomainParser
  RequestMatchers
  FrontendStatistic
  UserIdController
  CommonModule
  FeConfigXSD
  FCGI
)
