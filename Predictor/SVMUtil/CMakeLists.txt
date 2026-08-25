add_executable(SVMUtil
  Application.cpp
)
target_link_libraries(SVMUtil
  PRIVATE
    Generics
    # IPMap
    #Geoip
    Vanga::GearsThreading
)
