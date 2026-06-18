add_executable(BiddingFrontendRequestInfoFillerPerfTest
  BiddingFrontendRequestInfoFillerPerfTest.cpp)

target_link_libraries(BiddingFrontendRequestInfoFillerPerfTest
  PRIVATE
    jemalloc
    BiddingFrontend
    Generics
)

target_include_directories(BiddingFrontendRequestInfoFillerPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
