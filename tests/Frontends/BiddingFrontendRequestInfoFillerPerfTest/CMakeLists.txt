add_executable(BiddingFrontendRequestInfoFillerPerfTest
  BiddingFrontendRequestInfoFillerPerfTest.cpp)

target_link_libraries(BiddingFrontendRequestInfoFillerPerfTest
  PRIVATE
    BiddingFrontend
    Generics
)

target_include_directories(BiddingFrontendRequestInfoFillerPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
