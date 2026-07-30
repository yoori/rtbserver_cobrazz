add_library(HttpServer STATIC
  HttpServer.cpp
  HttpServer.hpp
)

target_link_libraries(HttpServer
  Generics
  Sync
  pthread
  Boost::headers
)
