add_executable(CTRModelPredictPerfTest
  CTRModelPredictPerfTest.cpp
)

target_link_libraries(CTRModelPredictPerfTest
  PRIVATE
    jemalloc
    CTRProviderImplLib
)

target_include_directories(CTRModelPredictPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
