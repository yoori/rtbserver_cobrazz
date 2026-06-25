add_executable(BiddingFrontendRequestInfoFillerPerfTest
  BiddingFrontendRequestInfoFillerPerfTest.cpp)

add_executable(BiddingFrontendFastRequestInfoFillerPerfTest
  BiddingFrontendRequestInfoFillerPerfTest.cpp)

target_link_libraries(BiddingFrontendRequestInfoFillerPerfTest
  PRIVATE
    jemalloc
    BiddingFrontend
    Generics
)

target_link_libraries(BiddingFrontendFastRequestInfoFillerPerfTest
  PRIVATE
    jemalloc
    BiddingFrontend
    Generics
)

target_include_directories(BiddingFrontendRequestInfoFillerPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_include_directories(BiddingFrontendFastRequestInfoFillerPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(BiddingFrontendRequestInfoFillerPerfTest
  PRIVATE
    REQUEST_INFO_FILLER_TEST_NAME="BiddingFrontendRequestInfoFillerPerfTest"
    REQUEST_INFO_FILLER_USE_FAST_JSON_PARSER=0
)

target_compile_definitions(BiddingFrontendFastRequestInfoFillerPerfTest
  PRIVATE
    REQUEST_INFO_FILLER_TEST_NAME="BiddingFrontendFastRequestInfoFillerPerfTest"
    REQUEST_INFO_FILLER_USE_FAST_JSON_PARSER=1
)
