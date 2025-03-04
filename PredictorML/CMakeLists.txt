add_library(PredictorML SHARED
  Predictor.cpp
)

target_link_libraries(PredictorML
  libcatboostmodel.so
)

add_executable(ClickModelConverter
  ClickModel/CsvConverter/CsvConverter.cpp
  ClickModel/CsvConverter/Main.cpp
)

target_link_libraries(ClickModelConverter
  Logger
)

add_executable(ClickModelBenchmark
  ClickModel/Benchmark/Benchmark.cpp
  ClickModel/Benchmark/Main.cpp
)

target_link_libraries(ClickModelBenchmark
  Logger
  PredictorML
)