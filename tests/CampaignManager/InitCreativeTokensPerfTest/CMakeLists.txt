add_executable(InitCreativeTokensPerfTest
  InitCreativeTokensPerfTest.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CreativeTextGenerator.cpp
)

target_link_libraries(InitCreativeTokensPerfTest
  PRIVATE
    jemalloc
    CreativeTemplateArgs
    Generics
    HTTP
)

target_include_directories(InitCreativeTokensPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
