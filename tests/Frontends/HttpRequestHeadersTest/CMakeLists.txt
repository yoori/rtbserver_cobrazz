add_executable(HttpRequestHeadersTest HttpRequestHeadersTest.cpp)

target_link_libraries(HttpRequestHeadersTest
  PRIVATE
    FrontendServerCommons
    FCGI
    HTTP
    Generics
)
