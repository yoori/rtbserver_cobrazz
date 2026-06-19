add_executable(CampaignManagerLoggerPerfTest
  CampaignManagerLoggerPerfTest.cpp
)

target_link_libraries(CampaignManagerLoggerPerfTest
  PRIVATE
    jemalloc
    CampaignManagerLogger
    Commons
    LogCommons
    Generics
    Logger
)

target_include_directories(CampaignManagerLoggerPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
