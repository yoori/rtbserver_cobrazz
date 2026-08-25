add_library(Allocator SHARED
  Allocator.cpp
)


target_link_libraries(Allocator
  PRIVATE
    Generics
)

add_pst(UserChannelBaseProfilePST  UserChannelBaseProfile.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoCommons)
add_pst(UserChannelHistoryProfilePST  UserChannelHistoryProfile.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoCommons)

add_library(ChannelMatcher SHARED
  ChannelMatcher.cpp
)
target_link_libraries(ChannelMatcher
  PRIVATE
    Generics
    UtilCommons
    UserChannelBaseProfilePST
    UserChannelHistoryProfilePST
)

add_pst(UserFreqCapProfileDescriptionPST  UserFreqCapProfileDescription.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoCommons)

add_library(UserFreqCapProfile SHARED
  UserFreqCapProfile.cpp
)
target_link_libraries(UserFreqCapProfile
  PRIVATE
    UserFreqCapProfileDescriptionPST
    Generics
    Allocator
    UtilCommons
)

add_pst(UserOperationProfilesPST  UserOperationProfiles.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoCommons)
