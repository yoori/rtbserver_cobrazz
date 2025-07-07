add_executable(UnitTestNew
  Coroutine/CoroutineTest.cpp
  LogCommons/ArchiveTest.cpp
  RequestInfoSvcs/ExpressionMatcher/ChannelMatcherTest.cpp
  RequestInfoSvcs/ExpressionMatcher/UserInventoryContainerTest.cpp
  UserInfoSvcs/UserBindServer/FetchableHashTableTest.cpp
  UserInfoSvcs/UserBindServer/UserBindChunkTypesTest.cpp
  UserInfoSvcs/UserInfoContainer/UserInfoContainerTest.cpp
  PredictorML/PredictorTest.cpp
  test_main.cpp
)

target_link_libraries(UnitTestNew
  GTest::gtest
  GTest::gtest_main
  ExpressionChannelMatcher
  ExpressionMatcherOutLogger
  LogCommons
  PredictorML
  ProfileMap
  UserBindContainer
  UserInfoContainer
  UserInventoryContainer
)

add_test(UnitTestNew UnitTestNew)