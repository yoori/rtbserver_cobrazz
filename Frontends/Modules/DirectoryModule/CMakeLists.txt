add_library(DirectoryModule SHARED
  DirectoryModule.cpp
)

target_link_libraries(DirectoryModule
  Stream
  Commons
  FeConfigXSD
  FCGI
)
