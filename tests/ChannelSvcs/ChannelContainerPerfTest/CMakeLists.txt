add_executable(ChannelContainerPerfTest ChannelContainerPerfTest.cpp)
add_executable(TriggerMatchResTest TriggerMatchResTest.cpp)

foreach(target ChannelContainerPerfTest TriggerMatchResTest)
  target_link_libraries(${target}
    PRIVATE
      ChannelContainer
      ChannelCommons
      CORBACommons
      ProcessControlIDL
      BLogic
      Generics
      HTTP
  )

  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
endforeach()
