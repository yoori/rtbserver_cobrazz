add_subdirectory(Grpc)
add_subdirectory(PmrTests)

add_executable(AsyncMutexTest
  AsyncMutexTest.cpp
)

add_executable(OrderedAsyncTaskWindowTest
  OrderedAsyncTaskWindowTest.cpp
)

add_executable(FastSchedulerPerfTest
  FastSchedulerPerfTest.cpp
)

target_link_libraries(AsyncMutexTest
  PRIVATE
    Commons
)

target_link_libraries(OrderedAsyncTaskWindowTest
  PRIVATE
    Commons
)

target_link_libraries(FastSchedulerPerfTest
  PRIVATE
    Commons
    Generics
)

target_include_directories(AsyncMutexTest
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
