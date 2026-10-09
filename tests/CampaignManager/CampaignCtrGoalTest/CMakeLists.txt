add_executable(CampaignCtrGoalTest
  CampaignCtrGoalTest.cpp
)

target_link_libraries(CampaignCtrGoalTest
  PRIVATE
    Boost::headers
    CampaignConfig
    HTTP
)
