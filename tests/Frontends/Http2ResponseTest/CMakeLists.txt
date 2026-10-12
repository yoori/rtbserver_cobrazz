add_executable(Http2ResponseTest Http2ResponseTest.cpp)

target_link_libraries(Http2ResponseTest
  PRIVATE
    FrontendServerCommons
    FCGI
    Logger
    Boost::headers
    Nghttp2::Nghttp2
)
