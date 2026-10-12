add_library(ChannelSessionFactory SHARED
  ChannelLoadSessionImpl.cpp
)
target_link_libraries(ChannelSessionFactory
  PRIVATE
    Generics
    Logger
    ChannelUpdateGrpcClient
)
