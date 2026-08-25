add_library(HttpServer STATIC
  HttpServer.cpp
  HttpServer.hpp
)

target_link_libraries(HttpServer
  PUBLIC
    Generics
  PRIVATE
    Threads::Threads
    Boost::headers
)
