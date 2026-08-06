ADD_SUBDIRECTORY(Grpc)
ADD_SUBDIRECTORY(PmrTests)

add_executable(AsyncMutexTest
  AsyncMutexTest.cpp
)

target_link_libraries(AsyncMutexTest
  PRIVATE
  Commons
)

target_include_directories(AsyncMutexTest
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
