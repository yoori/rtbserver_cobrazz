add_idl(StatsCollectorIDL StatsCollector.idl ${IDL_DIRECTORY}/Controlling/StatsCollector)

target_link_libraries(StatsCollectorIDL StatsIDL)
add_executable(StatsCollector

 StatsCollectorImpl.cpp
            StatsCollectorMain.cpp
            StatsCollectorAgent.cpp
)
target_link_libraries(StatsCollector
 Commons
 CORBACommons
 ProcessControlVars
 XMLUtility
 StatsCollectorIDL
 SNMPAgent
 StatsCollectorConfigXSD
)

#install(TARGETS StatsCollectorIDL StatsCollector  DESTINATION ${INSTALL_DIR})