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


install(TARGETS UserBindServerIDL  DESTINATION ${INSTALL_DIR})