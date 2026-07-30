add_executable(BiddingFrontendHashMapPerfTest
  BiddingFrontendHashMapPerfTest.cpp
)

target_link_libraries(BiddingFrontendHashMapPerfTest
  PRIVATE
    jemalloc
    Generics
)

target_include_directories(BiddingFrontendHashMapPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
