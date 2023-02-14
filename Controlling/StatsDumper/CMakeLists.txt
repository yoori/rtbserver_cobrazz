
add_library(StatsDumper SHARED

StatsDumper.cpp
)
target_link_libraries(StatsDumper
 Commons
 CORBACommons
 ProcessControlVars
 XMLUtility
 StatsCollectorIDL
 SNMPAgent
 StatsCollectorConfigXSD
)

#install(TARGETS  StatsDumper DESTINATION ${INSTALL_DIR})