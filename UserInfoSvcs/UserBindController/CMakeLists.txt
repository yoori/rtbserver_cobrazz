add_idl(UserBindControllerIDL UserBindController.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserBindController)

target_link_libraries(UserBindControllerIDL
  Commons
  CORBACommons
  CorbaTypesIDL
  UserBindServerIDL
)

add_library(UserBindControllerStubs SHARED
  UserBindOperationDistributor.cpp
)

target_link_libraries(UserBindControllerStubs
  CORBACommons
  Commons
  CorbaTypesIDL 
  UserBindServerIDL 
  UserBindControllerIDL
)

add_executable(UserBindController
  UserBindControllerMain.cpp 
  UserBindControllerImpl.cpp
)

target_link_libraries(UserBindController
  UserBindControllerIDL
  CORBACommons
  Commons
  CorbaTypesIDL
  UserBindServerIDL
  UserBindControllerStubs
  UserBindControllerConfigXSD
  ProcessControlVars
)

install(TARGETS UserBindControllerIDL UserBindControllerStubs UserBindController DESTINATION ${INSTALL_DIR})
