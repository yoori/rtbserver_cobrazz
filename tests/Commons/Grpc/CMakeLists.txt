add_executable(RefPoolTest
  RefPoolTest.cpp
)

add_executable(DistributedPartitionPoolTest
  DistributedPartitionPoolTest.cpp
)

add_executable(BatchPartialResponseTest
  BatchPartialResponseTest.cpp
)

target_link_libraries(RefPoolTest
  PRIVATE
    Commons
    Generics
)

target_link_libraries(DistributedPartitionPoolTest
  PRIVATE
    Commons
    Generics
    Logger
    Sync
)

target_link_libraries(BatchPartialResponseTest
  PRIVATE
    Commons
    Generics
    GrpcBatch
)

target_include_directories(RefPoolTest
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

target_include_directories(DistributedPartitionPoolTest
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

target_include_directories(BatchPartialResponseTest
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
