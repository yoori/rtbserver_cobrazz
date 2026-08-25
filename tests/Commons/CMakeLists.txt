ADD_SUBDIRECTORY(Grpc)
ADD_SUBDIRECTORY(PmrTests)

add_executable(AsyncMutexTest
  AsyncMutexTest.cpp
)

add_executable(FastSchedulerPerfTest
  FastSchedulerPerfTest.cpp
)

target_link_libraries(AsyncMutexTest
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
