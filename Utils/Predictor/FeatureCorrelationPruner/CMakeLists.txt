add_library(FeatureCorrelationPrunerLib
  CorrelationPruner.cpp
)

target_include_directories(FeatureCorrelationPrunerLib
  PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(FeatureCorrelationPrunerLib
  PUBLIC
    Generics
)

add_executable(FeatureCorrelationPruner
  Application.cpp
)

target_link_libraries(FeatureCorrelationPruner
  PRIVATE
    FeatureCorrelationPrunerLib
)
