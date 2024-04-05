add_library(Allocator SHARED
  Allocator.cpp
)

target_link_libraries(Allocator
  Generics
  Commons
)

add_pst(UserChannelBaseProfilePST  UserChannelBaseProfile.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoCommons)
add_pst(UserChannelHistoryProfilePST  UserChannelHistoryProfile.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoCommons)

add_library(ChannelMatcher SHARED
  ChannelMatcher.cpp
)

target_link_libraries(ChannelMatcher
  Generics
  PlainStorage2
  PlainStorage3
  UserChannelBaseProfilePST
  UserChannelHistoryProfilePST
  UtilCommons
)

add_pst(UserFreqCapProfileDescriptionPST  UserFreqCapProfileDescription.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoCommons)

add_library(UserFreqCapProfile SHARED
  UserFreqCapProfile.cpp
)

target_link_libraries(UserFreqCapProfile
  Allocator
  Commons
  Generics
  UserFreqCapProfileDescriptionPST
  UtilCommons
)

add_library(UserInfoStatistics SHARED
  Statistics.cpp
)

target_link_libraries(UserInfoStatistics
  UServerUtils
)

add_pst(UserOperationProfilesPST  UserOperationProfiles.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoCommons)