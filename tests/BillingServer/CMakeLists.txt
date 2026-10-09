add_executable(BillingContainerDumpTest
  BillingContainerDumpTest.cpp
)

target_link_libraries(BillingContainerDumpTest
  PRIVATE
    BillingContainer
    Commons
    LogCommons
)

add_custom_target(RunBillingContainerDumpTest
  COMMAND $<TARGET_FILE:BillingContainerDumpTest>
  DEPENDS BillingContainerDumpTest
)
