add_executable(ChannelContainerPerfTest ChannelContainerPerfTest.cpp)
add_executable(TriggerMatchResTest TriggerMatchResTest.cpp)
add_executable(UrlPrefixIndexTest UrlPrefixIndexTest.cpp)

foreach(target ChannelContainerPerfTest TriggerMatchResTest UrlPrefixIndexTest)
  target_link_libraries(${target}
    PRIVATE
      ChannelContainer
      ChannelCommons
      BLogic
      Generics
      HTTP
  )
endforeach()
