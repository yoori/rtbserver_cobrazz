

add_executable(ProfileDump
 Application.cpp
)
target_link_libraries(ProfileDump

 Generics
 PlainStorage2

 UtilCommons
 ChannelMatcher
 RequestInfoManagerCompatibility
 UserBaseProfileAdapter
 UserHistoryProfileAdapter
 UserFreqCapProfileAdapter
Allocator

)

install(TARGETS ProfileDump  DESTINATION ${INSTALL_DIR})