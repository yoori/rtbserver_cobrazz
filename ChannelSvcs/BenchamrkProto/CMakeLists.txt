add_executable(BenchmarkProto
  Main.cpp
)

include(${UNIXCOMMONS}/src/UServerUtils/Grpc/Generator/GrpcTargets.cmake)
add_grpc_library(TestProto
  PROTOS
    Test.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(BenchmarkProto
  TestProto
)