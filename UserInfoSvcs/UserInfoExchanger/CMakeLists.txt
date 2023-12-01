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
  AdServerCommonsXSD
  Allocator
  CORBACommons
  CORBAConfigParser
  Commons
  PlainStorage3
  ProcessControlVars
  UServerUtils
  UserInfoExchangerConfigXSD
  UserInfoExchangerIDL
)