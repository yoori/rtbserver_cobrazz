add_executable(UserNavigationContainerTest
  UserNavigationContainerTest.cpp
)

target_link_libraries(UserNavigationContainerTest
  PRIVATE
    UserNavigationContainer
    UserNavigationProfilePST
    Commons
    Generics
    Logger
    ProfileMap
    ProfileMapRocksDB
)

target_include_directories(UserNavigationContainerTest
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
