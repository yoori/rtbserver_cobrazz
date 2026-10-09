add_executable(UserBindArenaFillTest
  UserBindArenaFillTest.cpp
)

target_link_libraries(UserBindArenaFillTest
  PRIVATE
    Generics
    UserBindServerGrpc
    protobuf::libprotobuf
)
