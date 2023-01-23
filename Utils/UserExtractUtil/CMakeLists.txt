#include_directories(/opt/foros/vanga/include)



add_executable(UserExtractUtil
    UserExtractUtil.cpp
 )
target_link_libraries(UserExtractUtil

 Generics

 Commons
 ProfileMap
 PlainStorage3
# ChannelInventory
 ExpressionMatcherCompatibility

)

install(TARGETS UserExtractUtil  DESTINATION ${INSTALL_DIR})