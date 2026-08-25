add_executable(Http2EchoServer
  Application.cpp
)

target_link_libraries(Http2EchoServer
  PRIVATE
    Generics
    Logger
    Sync
    FCGI
    FrontendServerCommons
)
