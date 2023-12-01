add_executable(ProfileDump
  Application.cpp
)

target_link_libraries(ProfileDump
  Allocator
  ChannelMatcher
  Generics
  PlainStorage2
  RequestInfoManagerCompatibility
  UServerUtils
  UserBaseProfileAdapter
  UserFreqCapProfileAdapter
  UserHistoryProfileAdapter
  UtilCommons
)