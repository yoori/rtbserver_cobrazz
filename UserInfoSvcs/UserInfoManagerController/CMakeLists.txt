add_idl(UserInfoClusterControlIDL
  UserInfoClusterControl.idl
  ${IDL_DIRECTORY}/UserInfoSvcs/UserInfoManagerController
)

target_link_libraries(UserInfoClusterControlIDL
  Generics
  CORBACommons
  ProcessControlIDL
  StatsIDL
)

add_idl(UserInfoManagerControllerIDL
  UserInfoManagerController.idl
  ${IDL_DIRECTORY}/UserInfoSvcs/UserInfoManagerController
)

target_link_libraries(UserInfoManagerControllerIDL
  UserInfoManagerIDL
  UserInfoManagerControlIDL
  ProcessControlIDL
)

add_executable(UserInfoManagerController
  UserInfoManagerControllerMain.cpp
  UserInfoClusterControlImpl.cpp
  UserInfoManagerControllerImpl.cpp
)

target_link_libraries(UserInfoManagerController
  CORBACommons
  ProcessControlVars
  Commons
  UserInfoManagerIDL
  UserInfoManagerControlIDL
  UserInfoManagerControllerIDL
  UserInfoClusterControlIDL
  AdServerCommonsXSD
  UserInfoManagerControllerConfigXSD
)

add_library(UserInfoManagerControllerCall SHARED
  UserInfoManagerSessionFactory.cpp
  UserInfoOperationDistributor.cpp
  UserInfoManagerSessionImpl.cpp
)

target_link_libraries(UserInfoManagerControllerCall
  Generics
  CORBACommons
  UserInfoManagerIDL
  UserInfoManagerControlIDL
  UserInfoClusterControlIDL
  UserInfoManagerControllerIDL
  pthread
  Logger
  Sync
)
