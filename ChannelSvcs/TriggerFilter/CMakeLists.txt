add_executable(TriggerFilter
    Application.cpp
)
target_link_libraries(TriggerFilter
 Generics
 Commons
 ChannelCommons
 ChannelContainer
)

install(TARGETS TriggerFilter  DESTINATION ${INSTALL_DIR})