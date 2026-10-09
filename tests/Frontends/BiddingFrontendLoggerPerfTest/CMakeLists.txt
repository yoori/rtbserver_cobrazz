add_executable(BiddingFrontendLoggerPerfTest
  BiddingFrontendLoggerPerfTest.cpp
)

target_link_libraries(BiddingFrontendLoggerPerfTest
  PRIVATE
    PkgConfig::Jemalloc
    BiddingFrontend
    Generics
    Logger
)
