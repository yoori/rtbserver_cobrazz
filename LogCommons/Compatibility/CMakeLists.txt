add_library(RequestCompatibility SHARED
 Request_Base.cpp 
  Request_v354.cpp 
  Request_v360.cpp 
  Request_v361.cpp

)

target_link_libraries(RequestCompatibility
 Generics
 Commons
 LogCommons
)
install(TARGETS RequestCompatibility  DESTINATION ${INSTALL_DIR})