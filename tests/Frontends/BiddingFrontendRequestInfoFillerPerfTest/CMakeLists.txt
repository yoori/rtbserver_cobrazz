function(add_bidding_request_info_filler_perf_test target)
  add_executable(${target}
    BiddingFrontendRequestInfoFillerPerfTest.cpp)

  target_link_libraries(${target}
    PRIVATE
      PkgConfig::Jemalloc
      BiddingFrontend
      CampaignServerIDL
      CommonModule
      Commons
      CORBACommons
      DomainConfigXSD
      DomainParser
      FeConfigXSD
      Generics
      HTTP
      Logger
      ProcessControlIDL
      Sync
      UserIdController
  )

  target_include_directories(${target}
    PRIVATE
      ${CMAKE_SOURCE_DIR}
  )

  target_compile_definitions(${target}
    PRIVATE
      REQUEST_INFO_FILLER_TEST_NAME="${target}"
  )
endfunction()

add_bidding_request_info_filler_perf_test(
  BiddingFrontendRequestInfoFillerPerfTest)
