add_library(KafkaProducer SHARED
  KafkaProducer.cpp
)
target_link_libraries(KafkaProducer
  AdServerCommonsXSD
  Commons
  rdkafka
  rdkafka++
)

add_executable(KafkaBenchmark
  KafkaBenchmark.cpp
)

target_link_libraries(KafkaBenchmark
  KafkaProducer
)

install(TARGETS KafkaProducer KafkaBenchmark DESTINATION ${INSTALL_DIR})