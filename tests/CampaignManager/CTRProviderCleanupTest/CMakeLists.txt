add_executable(CTRProviderCleanupTest
  CTRProviderCleanupTest.cpp
)

target_link_libraries(CTRProviderCleanupTest
  PRIVATE
    Boost::headers
    CTRProvider
    HTTP
    CampaignConfig
    Generics
)

add_custom_target(RunCTRProviderCleanupTest
  COMMAND $<TARGET_FILE:CTRProviderCleanupTest>
  DEPENDS CTRProviderCleanupTest
)
