add_executable(UserInfoExchangerProxy
  UserInfoExchangerProxyMain.cpp 
  UserInfoExchangerProxyImpl.cpp 
)

target_link_libraries(UserInfoExchangerProxy
 CORBACommons
 ProcessControlVars
 Commons
 AdServerCommonsXSD
 UserInfoExchangerProxyConfigXSD
 UserInfoExchangerIDL
)

install(TARGETS UserInfoExchangerProxy DESTINATION ${INSTALL_DIR})