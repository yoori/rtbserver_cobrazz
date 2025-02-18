add_executable(UnitTestNew
  Coroutine/CoroutineTest.cpp
  LogCommons/ArchiveTest.cpp
  RequestInfoSvcs/ExpressionMatcher/ChannelMatcherTest.cpp
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
  LogCommons
  PredictorML
  ProfileMap
  UserBindContainer
  UserInfoContainer
)

add_test(UnitTestNew UnitTestNew)