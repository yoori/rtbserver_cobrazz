
add_executable(SVMUtil
Application.cpp
)
target_link_libraries(SVMUtil
 Generics

 Commons
# IPMap
#Geoip
/opt/foros/vanga/lib/libGearsBasic.so
/opt/foros/vanga/lib/libGearsThreading.so
)
#install(TARGETS SVMUtil  DESTINATION ${INSTALL_DIR})


