add_library(KafkaProducer SHARED
KafkaProducer.cpp
)
target_link_libraries(KafkaProducer
 AdServerCommonsXSD
 Commons
 rdkafka
 rdkafka++
)

install(TARGETS KafkaProducer DESTINATION ${INSTALL_DIR})