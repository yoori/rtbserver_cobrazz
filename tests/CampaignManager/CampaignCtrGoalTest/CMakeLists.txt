add_executable(CampaignCtrGoalTest
  CampaignCtrGoalTest.cpp
)

target_link_libraries(CampaignCtrGoalTest
  PRIVATE
    Boost::headers
    CampaignConfig
    HTTP
)

target_include_directories(CampaignCtrGoalTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
