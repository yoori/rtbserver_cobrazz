add_executable(UnitTestNew
  UserInfoSvcs/UserInfoContainer/user_info_container_test.cpp
  test_main.cpp
)

target_link_libraries(UnitTestNew
  GTest::gtest
  GTest::gtest_main
  ProfileMap
  UserInfoContainer
)

add_test(UnitTestNew UnitTestNew)