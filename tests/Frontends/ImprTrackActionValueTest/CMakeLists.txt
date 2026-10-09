add_executable(ImprTrackActionValueTest ImprTrackActionValueTest.cpp)

target_link_libraries(ImprTrackActionValueTest
  PRIVATE
    ImprTrackFrontend
    CommonModule
    CampaignManagerGrpc
    CampaignServerIDL
    CORBACommons
    FCGI
    Logger
    ProcessControlIDL
)
