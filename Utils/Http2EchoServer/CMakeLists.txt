add_executable(Http2EchoServer
  Application.cpp
)

target_link_libraries(Http2EchoServer
  Generics
  Stream
  Commons
  FCGI
  FrontendServerCommons
  Boost::system
  nghttp2
)
