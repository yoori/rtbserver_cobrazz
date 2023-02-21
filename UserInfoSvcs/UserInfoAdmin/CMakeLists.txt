add_executable(UserInfoAdmin
    Application.cpp
)
target_link_libraries(UserInfoAdmin
 Generics
 CORBACommons

 UtilCommons
 ChannelMatcher
 UserFreqCapProfile

 UserInfoManagerIDL
 UserInfoManagerControlIDL
 UserInfoManagerControllerIDL
 UserInfoExchangerIDL
    UserInfoManagerControllerL

)

#install(TARGETS UserInfoAdmin DESTINATION ${INSTALL_DIR})