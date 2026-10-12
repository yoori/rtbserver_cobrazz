add_executable(CampaignManagerLoggerPerfTest
  CampaignManagerLoggerPerfTest.cpp
)

target_link_libraries(CampaignManagerLoggerPerfTest
  PRIVATE
    PkgConfig::Jemalloc
    CampaignManagerLogger
    HTTP
    Generics
)
