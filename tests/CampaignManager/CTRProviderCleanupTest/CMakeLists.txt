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

target_include_directories(CTRProviderCleanupTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_custom_target(RunCTRProviderCleanupTest
  COMMAND $<TARGET_FILE:CTRProviderCleanupTest>
  DEPENDS CTRProviderCleanupTest
)
