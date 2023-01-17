

add_executable(UIDGeneratorUtil
 Application.cpp
)
target_link_libraries(UIDGeneratorUtil

 Generics
 PlainStorage2

 UtilCommons
 ChannelMatcher
# RequestInfoCommons
# ProfilingCommons
 RequestInfoManagerCompatibility
 UserBaseProfileAdapter
 UserHistoryProfileAdapter
 UserFreqCapProfileAdapter
Allocator
protobuf
protoc
UIDGeneratorProtocolPB

)

install(TARGETS UIDGeneratorUtil DESTINATION ${INSTALL_DIR})