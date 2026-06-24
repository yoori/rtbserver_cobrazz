add_executable(BiddingFrontendCoreTest
  BiddingFrontendCoreTest.cpp)

target_link_libraries(BiddingFrontendCoreTest
  PRIVATE
    jemalloc
    BiddingFrontend
    FrontendServerCommons
    CommonModule
    Generics
)

target_include_directories(BiddingFrontendCoreTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
