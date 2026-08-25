add_library(KafkaProducer SHARED
  KafkaProducer.cpp
)
target_link_libraries(KafkaProducer
  PRIVATE
    AdServerCommonsXSD
    PkgConfig::RdKafka
)
