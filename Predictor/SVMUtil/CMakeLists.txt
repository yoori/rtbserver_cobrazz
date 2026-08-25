
add_executable(SVMUtil
  Application.cpp
)
target_link_libraries(SVMUtil
  PRIVATE
    Generics

    # IPMap
    #Geoip
    /opt/foros/vanga/lib/libGearsThreading.so
    /opt/foros/vanga/lib/libGearsBasic.so
)
#install(TARGETS SVMUtil  DESTINATION ${INSTALL_DIR})
