
add_executable(ChannelSearchAdmin
  Application.cpp
)
target_link_libraries(ChannelSearchAdmin
  Generics
  Logger
  Commons
  UtilCommons
  ChannelSearchServiceIDL
  ChannelManagerControllerIDL  ChannelClusterControlIDL
  ChannelSessionFactory
)
