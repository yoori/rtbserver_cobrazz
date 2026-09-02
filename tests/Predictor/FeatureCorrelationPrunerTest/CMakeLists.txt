add_executable(FeatureCorrelationPrunerTest
  FeatureCorrelationPrunerTest.cpp
)

target_link_libraries(FeatureCorrelationPrunerTest
  PRIVATE
    FeatureCorrelationPrunerLib
)

add_executable(FeatureCorrelationPrunerPerfTest
  FeatureCorrelationPrunerPerfTest.cpp
)

target_link_libraries(FeatureCorrelationPrunerPerfTest
  PRIVATE
    FeatureCorrelationPrunerLib
)

add_custom_target(RunFeatureCorrelationPrunerTest
  COMMAND $<TARGET_FILE:FeatureCorrelationPrunerTest>
  DEPENDS FeatureCorrelationPrunerTest
)

add_custom_target(RunFeatureCorrelationPrunerPerfTest
  COMMAND $<TARGET_FILE:FeatureCorrelationPrunerPerfTest>
  DEPENDS FeatureCorrelationPrunerPerfTest
)
