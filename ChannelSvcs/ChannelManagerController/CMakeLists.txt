add_library(ChannelSessionFactory SHARED
  ChannelLoadSessionFactory.cpp
  ChannelLoadSessionImpl.cpp
)
target_link_libraries(ChannelSessionFactory
  PRIVATE
    Generics
    Logger
    CORBACommons
    ChannelUpdateBaseIDL
)
