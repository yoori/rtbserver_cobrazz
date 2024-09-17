add_executable(UnitTestNew
  LogCommons/ArchiveTest.cpp
  RequestInfoSvcs/ExpressionMatcher/ChannelMatcherTest.cpp
  UserInfoSvcs/UserBindServer/FetchableHashTableTest.cpp
  UserInfoSvcs/UserBindServer/UserBindChunkTypesTest.cpp
  UserInfoSvcs/UserInfoContainer/UserInfoContainerTest.cpp
  test_main.cpp
)

target_link_libraries(UnitTestNew
  GTest::gtest
  GTest::gtest_main
  ExpressionChannelMatcher
  LogCommons
  ProfileMap
  UserBindContainer
  UserInfoContainer
)

add_test(UnitTestNew UnitTestNew)