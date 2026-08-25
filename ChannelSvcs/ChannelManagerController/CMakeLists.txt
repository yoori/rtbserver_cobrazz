add_idl(ChannelClusterControlIDL ChannelClusterControl.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelManagerController VALUETYPE)
target_link_libraries(ChannelClusterControlIDL
  PRIVATE
    ProcessControlIDL
)

add_idl(ChannelManagerControllerIDL ChannelManagerController.idl ${IDL_DIRECTORY}/ChannelSvcs/ChannelManagerController VALUETYPE)
target_link_libraries(ChannelManagerControllerIDL
  PRIVATE
    ChannelCommonsIDL
    ChannelUpdateBaseIDL
    ChannelServerIDL
)

add_library(ChannelSessionFactory SHARED
  ChannelLoadSessionFactory.cpp
  ChannelServerSessionFactory.cpp
  ChannelSessionFactory.cpp
  ChannelLoadSessionImpl.cpp
)
target_link_libraries(ChannelSessionFactory
  PRIVATE
    Generics
    Logger
    CORBACommons
    ChannelManagerControllerIDL
)
add_dependencies(ChannelSessionFactory
  ChannelCommons
  ChannelClusterControlIDL)

add_library(ChannelClusterControl SHARED
  ChannelClusterControlImpl.cpp
)
target_link_libraries(ChannelClusterControl
  PRIVATE
    Generics
    CORBACommons
    ChannelClusterControlIDL
)
