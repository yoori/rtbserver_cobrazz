add_executable(ChannelContainerPerfTest ChannelContainerPerfTest.cpp)
add_executable(TriggerMatchResTest TriggerMatchResTest.cpp)
add_executable(UrlPrefixIndexTest UrlPrefixIndexTest.cpp)
add_executable(LongestAnchorTest LongestAnchorTest.cpp)

foreach(target ChannelContainerPerfTest TriggerMatchResTest UrlPrefixIndexTest LongestAnchorTest)
  target_link_libraries(${target}
    PRIVATE
      ChannelContainer
      ChannelCommons
      BLogic
      Generics
      HTTP
  )
endforeach()
