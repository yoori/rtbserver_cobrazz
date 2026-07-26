add_executable(UserBindArenaFillTest
  UserBindArenaFillTest.cpp
)

target_link_libraries(UserBindArenaFillTest
  PRIVATE
    Generics
    UserBindServerGrpc
    protobuf::libprotobuf
)

target_include_directories(UserBindArenaFillTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${PB_DIRECTORY}
)
