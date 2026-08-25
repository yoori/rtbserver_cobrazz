add_executable(CTRModelPredictPerfTest
  CTRModelPredictPerfTest.cpp
)

target_link_libraries(CTRModelPredictPerfTest
  PRIVATE
    jemalloc
    CTRProviderImplLib
    HTTP
    CampaignConfig
    Generics
)

target_include_directories(CTRModelPredictPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_custom_target(CTRModelPredictCompatibilityTest
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/CatBoostRuntime
    --count=1
    --expected-ctr=0.00242545
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/CatBoostLogitSum
    --count=1
    --expected-ctr=0.00000591
  DEPENDS CTRModelPredictPerfTest
)
