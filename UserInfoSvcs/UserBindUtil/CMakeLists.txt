add_executable(UserBindUtil
    Application.cpp
)

target_link_libraries(UserBindUtil


 Generics
 CORBACommons

 LogCommons
 Commons
 PlainStorage3


)

#install(TARGETS UserBindUtil  DESTINATION ${INSTALL_DIR})