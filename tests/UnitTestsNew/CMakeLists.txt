add_executable(UnitTestNew
  RequestInfoSvcs/ExpressionMatcher/ChannelMatcherTest.cpp
  UserInfoSvcs/UserInfoContainer/UserInfoContainerTest.cpp
  test_main.cpp
)

target_link_libraries(UnitTestNew
  GTest::gtest
  GTest::gtest_main
  ExpressionChannelMatcher
  ProfileMap
  UserInfoContainer
)

add_test(UnitTestNew UnitTestNew)