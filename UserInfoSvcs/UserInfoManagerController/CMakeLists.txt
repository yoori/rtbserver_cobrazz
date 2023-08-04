add_idl(UserInfoClusterControlIDL UserInfoClusterControl.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserInfoManagerController)

target_link_libraries(UserInfoClusterControlIDL
  CORBACommons
  Generics
  ProcessControlIDL
  StatsIDL
)

add_idl(UserInfoManagerControllerIDL UserInfoManagerController.idl ${IDL_DIRECTORY}/UserInfoSvcs/UserInfoManagerController)

target_link_libraries(UserInfoManagerControllerIDL
  ProcessControlIDL
  UserInfoManagerControlIDL
  UserInfoManagerIDL
)

add_executable(UserInfoManagerController
  UserInfoClusterControlImpl.cpp
  UserInfoManagerControllerImpl.cpp
  UserInfoManagerControllerMain.cpp
)

target_link_libraries(UserInfoManagerController
  AdServerCommonsXSD
  CORBACommons
  Commons
  ProcessControlVars
  UserInfoClusterControlIDL
  UserInfoManagerControlIDL
  UserInfoManagerControllerConfigXSD
  UserInfoManagerControllerIDL
  UserInfoManagerIDL
)

add_library(UserInfoManagerControllerL SHARED
  GrpcUserInfoOperationDistributor.cpp
  UserInfoManagerSessionImpl.cpp
  UserInfoManagerSessionFactory.cpp
  UserInfoOperationDistributor.cpp
)

target_link_libraries(UserInfoManagerControllerL
  CORBACommons
  Generics
  Logger
  Sync
  UserInfoClusterControlIDL
  UserInfoManagerControlIDL
  UserInfoManagerControllerIDL
  UserInfoManagerIDL
  UServerUtils
  UserInfoManagerProto
  pthread
)