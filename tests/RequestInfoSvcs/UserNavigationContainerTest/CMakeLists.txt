add_executable(UserNavigationContainerTest
  UserNavigationContainerTest.cpp
)

target_link_libraries(UserNavigationContainerTest
  PRIVATE
    UserNavigationContainer
    UserNavigationProfileAdapter
    UserNavigationProfile_v1PST
    UserNavigationProfile_v2PST
    UserNavigationProfilePST
    Commons
    Generics
    Logger
    ProfileMap
    ProfileMapRocksDB
)
