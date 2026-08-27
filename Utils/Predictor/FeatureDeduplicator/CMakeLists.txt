add_executable(FeatureDeduplicator
  Application.cpp
)

target_link_libraries(FeatureDeduplicator
  PRIVATE
    Generics
)
