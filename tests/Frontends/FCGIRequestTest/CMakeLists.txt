add_executable(FCGIRequestTest FCGIRequestTest.cpp)

target_link_libraries(FCGIRequestTest
  PRIVATE
    FrontendServerCommons
    FCGI
    Commons
    Generics
    Logger
    Sync
)
