add_executable(InitCreativeTokensPerfTest
  InitCreativeTokensPerfTest.cpp
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/CampaignManager/CreativeTextGenerator.cpp
)

target_link_libraries(InitCreativeTokensPerfTest
  PRIVATE
    PkgConfig::Jemalloc
    CreativeTemplateArgs
    Generics
    HTTP
)
