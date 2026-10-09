add_executable(BiddingFrontendHashMapPerfTest
  BiddingFrontendHashMapPerfTest.cpp
)

target_link_libraries(BiddingFrontendHashMapPerfTest
  PRIVATE
    PkgConfig::Jemalloc
    Generics
    Boost::headers
)
