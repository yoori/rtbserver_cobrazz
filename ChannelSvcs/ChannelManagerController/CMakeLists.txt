add_idl(ChannelClusterControlIDL ChannelClusterControl.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelManagerController)
target_link_libraries(ChannelClusterControlIDL
 XMLUtility
 CORBACommons
 ProcessControlVars
 AdServerCommonsXSD
 Commons
 ChannelCommonsIDL
 ChannelUpdateBaseIDL
 ChannelServerIDL
 ChannelCommons
 )

add_idl(ChannelManagerControllerIDL ChannelManagerController.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelManagerController)
target_link_libraries(ChannelManagerControllerIDL

 XMLUtility
 CORBACommons
 ProcessControlVars
 AdServerCommonsXSD
 Commons
 ChannelCommonsIDL
 ChannelUpdateBaseIDL
 ChannelServerIDL
 ChannelCommons
)

add_executable( ChannelController
    ChannelControllerMain.cpp 
    ChannelControllerImpl.cpp
)
target_link_libraries(ChannelController
ChannelSessionFactory
ChannelClusterControl
ChannelClusterControlIDL
ChannelManagerControllerIDL
StatsIDL
ChannelManagerControllerConfigXSD
)

add_library(ChannelSessionFactory SHARED
  ChannelLoadSessionFactory.cpp 
  ChannelServerSessionFactory.cpp 
  ChannelSessionFactory.cpp 
  ChannelLoadSessionImpl.cpp
)
target_link_libraries(ChannelSessionFactory
 Generics
 Logger
 CORBACommons
 ChannelCommons
 ChannelClusterControlIDL
 ChannelManagerControllerIDL

)

add_library(ChannelClusterControl SHARED
ChannelClusterControlImpl.cpp
)
target_link_libraries(ChannelClusterControl
 Generics
 CORBACommons
ChannelClusterControlIDL
)

#install(TARGETS ChannelClusterControl ChannelSessionFactory ChannelController ChannelClusterControlIDL ChannelManagerControllerIDL   DESTINATION ${INSTALL_DIR})