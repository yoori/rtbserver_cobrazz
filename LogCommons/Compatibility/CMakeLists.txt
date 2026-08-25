add_library(RequestCompatibility SHARED
  Request_Base.cpp
  Request_v361.cpp
  Request_v371.cpp

)

target_link_libraries(RequestCompatibility
  PRIVATE
    Generics
    LogCommons
)
