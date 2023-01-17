add_library(Oracle SHARED

 OraException.cpp 
  Environment.cpp 
  Connection.cpp 
  Statement.cpp 
  ResultSet.cpp 
  TimeoutControl.cpp 
  ConnectionPool.cpp
)
target_link_libraries(Oracle
    Generics
    Commons
)


install(TARGETS Oracle DESTINATION ${INSTALL_DIR})
