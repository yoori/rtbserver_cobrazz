add_idl(UserInfoExchangerIDL  UserInfoExchanger.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserInfoExchanger)


target_link_libraries(UserInfoExchangerIDL
 CorbaTypesIDL
)

add_executable(UserInfoExchanger
  UserInfoExchangerMain.cpp 
  UserInfoExchangerImpl.cpp 
  UserInfoExchangePool.cpp
)

target_link_libraries(UserInfoExchanger
 CORBACommons
 CORBAConfigParser
 ProcessControlVars
 PlainStorage3
 Allocator
 Commons
 AdServerCommonsXSD
 UserInfoExchangerConfigXSD
 UserInfoExchangerIDL
 
)

#install(TARGETS UserInfoExchangerIDL UserInfoExchanger  DESTINATION ${INSTALL_DIR})