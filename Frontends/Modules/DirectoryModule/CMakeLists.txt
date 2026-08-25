add_library(DirectoryModule SHARED
  DirectoryModule.cpp
)

target_link_libraries(DirectoryModule
  PRIVATE
    Commons
    FeConfigXSD
    FCGI
)
