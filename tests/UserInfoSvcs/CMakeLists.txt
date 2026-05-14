add_subdirectory(UserBindPerf)
add_subdirectory(RocksDBProfileMapTest)

add_executable(UserBindContainerTest
  ../UnitTests/UserInfoSvcs/UserBindContainerTest/UserBindContainerTest.cpp
)

target_link_libraries(UserBindContainerTest
  PRIVATE
    UserBindContainer
    TestCommons
    Generics
    Commons
    ProfileMap
    ProfileMapRocksDB
)

target_include_directories(UserBindContainerTest
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
