add_library(PredictorML SHARED
  Predictor.cpp
)

target_link_libraries(PredictorML
  libcatboostmodel.so
)