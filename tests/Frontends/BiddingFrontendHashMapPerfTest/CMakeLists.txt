add_executable(BiddingFrontendHashMapPerfTest
  BiddingFrontendHashMapPerfTest.cpp
)

target_link_libraries(BiddingFrontendHashMapPerfTest
  PRIVATE
    PkgConfig::Jemalloc
    Generics
    Boost::headers
)

target_include_directories(BiddingFrontendHashMapPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
