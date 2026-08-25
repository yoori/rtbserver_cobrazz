add_library(Postgres SHARED
  ResultSet.cpp
  Statement.cpp
  Environment.cpp
  Connection.cpp
  ConnectionPool.cpp
  SqlStream.cpp
  Lob.cpp
)
target_link_libraries(Postgres
  PUBLIC
  pq

  PRIVATE
  Generics
  Commons
)

#install(TARGETS Postgres DESTINATION ${INSTALL_DIR})
