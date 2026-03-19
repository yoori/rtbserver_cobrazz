add_executable(FCGIEchoServer
  Application.cpp
)

target_link_libraries(FCGIEchoServer
  FCGI
  FrontendServerCommons
  Commons
  Generics
)
