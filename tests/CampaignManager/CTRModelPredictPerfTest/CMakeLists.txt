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

add_executable(VangaCTREvaluatorTest VangaCTREvaluatorTest.cpp)

target_link_libraries(VangaCTREvaluatorTest
  PRIVATE
    CTRProviderImplLib
    Vanga::DTree
    HTTP
    Generics
)

add_custom_target(RunVangaCTREvaluatorTest
  COMMAND $<TARGET_FILE:VangaCTREvaluatorTest>
    ${CMAKE_CURRENT_SOURCE_DIR}/vanga_feature_indexes.model
    ${CMAKE_CURRENT_SOURCE_DIR}/vanga_numeric_features.model
  DEPENDS VangaCTREvaluatorTest
)

add_custom_target(CTRModelPredictCompatibilityTest
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/VangaNumericSSP
    --mode=full
    --count=2
    --expected-ctr=0.88079707
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/VangaSSP
    --mode=full
    --count=2
    --expected-ctr=0.88079707
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/VangaSigmoid
    --mode=full
    --count=2
    --expected-ctr=0.88079707
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/VangaLogitSum
    --mode=full
    --count=2
    --expected-ctr=0.37754066
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/CatBoostRuntime
    --mode=full
    --count=1
    --expected-ctr=0.00242545
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/CatBoostLogitSum
    --mode=full
    --count=1
    --expected-ctr=0.00000591
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/CatBoostCampaignLogitSum
    --mode=full
    --campaign-id=236995
    --count=1
    --expected-ctr=0.00011987
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/CatBoostCampaignLogitSum
    --mode=full
    --campaign-id=236996
    --count=1
    --expected-ctr=0.00242545
  COMMAND $<TARGET_FILE:CTRModelPredictPerfTest>
    --model=${CMAKE_SOURCE_DIR}/tests/Predictor/data/CatBoostCampaignLogitSum
    --mode=switch-campaigns
    --campaign-id=236995
    --count=2
    --expected-ctr=0.00242545
  DEPENDS CTRModelPredictPerfTest
)
