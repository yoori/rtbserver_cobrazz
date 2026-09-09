add_pst(UserNavigationProfile_v1PST
  UserNavigationProfile_v1.pst
  ${PST_DIRECTORY}/RequestInfoSvcs/ExpressionMatcher/Compatibility)

add_library(UserNavigationProfileAdapter STATIC
  UserNavigationProfileAdapter.cpp
)

target_link_libraries(UserNavigationProfileAdapter
  PRIVATE
    UserNavigationProfilePST
    UserNavigationProfile_v1PST
    Generics
)
