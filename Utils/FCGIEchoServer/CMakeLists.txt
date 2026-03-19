add_executable(FCGIEchoServer
  Application.cpp
)

target_link_libraries(FCGIEchoServer
  Generics
  Commons
  FCGI
  FrontendServerCommons
)
