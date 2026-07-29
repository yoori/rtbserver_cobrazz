add_executable(BiddingFrontendLoggerPerfTest
  BiddingFrontendLoggerPerfTest.cpp
)

target_link_libraries(BiddingFrontendLoggerPerfTest
  PRIVATE
    jemalloc
    BiddingFrontend
    Commons
    LogCommons
    Generics
    Logger
)

target_include_directories(BiddingFrontendLoggerPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
