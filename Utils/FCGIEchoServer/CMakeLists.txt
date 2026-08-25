add_executable(FCGIEchoServer
  Application.cpp
)

target_link_libraries(FCGIEchoServer
  PRIVATE
    FCGI
    FrontendServerCommons
    Generics
    Sync
)
