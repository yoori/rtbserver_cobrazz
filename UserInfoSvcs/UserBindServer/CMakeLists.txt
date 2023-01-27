add_idl(UserBindServerIDL UserBindServer.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserBindServer)

target_link_libraries(UserBindServerIDL
  Commons
  CORBACommons
  CorbaTypesIDL
)

add_library(UserBindContainer STATIC
  ExternalIdHashAdapter.cpp 
  ChunkUtils.cpp 
  UserBindContainer.cpp 
  UserBindChunk.cpp 
  BindRequestContainer.cpp 
  BindRequestChunk.cpp
)

target_link_libraries(UserBindContainer
  Generics
  PrivacyFilter
  Commons
  LogCommons
  ProfileMap
)

add_pst(UserBindOperationProfilePST UserBindOperationProfile.pst ${PST_DIRECTORY}/UserInfoSvcs/UserBindServer)

add_executable(UserBindServer
  UserBindServerMain.cpp
  UserBindServerImpl.cpp
  UserBindOperationSaver.cpp
  UserBindOperationLoader.cpp
)

target_link_libraries(UserBindServer
  CORBACommons
  PrivacyFilter
  ProcessControlVars
  Commons
  ProfileMap
  MessageSaver
  LogCommons
  UserBindContainer
  UserBindServerConfigXSD
  UserBindServerIDL
)

#install(TARGETS UserBindServerIDL DESTINATION ${INSTALL_DIR})
