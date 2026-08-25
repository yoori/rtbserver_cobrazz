add_executable(CTRModelPredictPerfTest
  CTRModelPredictPerfTest.cpp
)

target_link_libraries(CTRModelPredictPerfTest
  PRIVATE
    PkgConfig::Jemalloc
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
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/CatBoostCampaignLogitSum
    --campaign-id=236995
    --count=1
    --expected-ctr=0.00011987
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/CatBoostCampaignLogitSum
    --campaign-id=236996
    --count=1
    --expected-ctr=0.00242545
  DEPENDS CTRModelPredictPerfTest
)
