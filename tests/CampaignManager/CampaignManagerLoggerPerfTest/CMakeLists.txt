add_executable(CampaignManagerLoggerPerfTest
  CampaignManagerLoggerPerfTest.cpp
)

target_link_libraries(CampaignManagerLoggerPerfTest
  PRIVATE
    TAO::PortableServer
    TAO::Valuetype
    jemalloc
    CampaignManagerLogger
    Commons
    CORBACommons
    HTTP
    LogCommons
    Generics
    Logger
    ProcessControlIDL
)

target_include_directories(CampaignManagerLoggerPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
