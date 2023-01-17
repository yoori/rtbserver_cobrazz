ADD_SUBDIRECTORY(PlainStorage)
ADD_SUBDIRECTORY(PlainStorage3)
ADD_SUBDIRECTORY(PlainStorageUtil)
ADD_SUBDIRECTORY(ProfileMap)

add_library(MessageSaver SHARED
MessageSaver.cpp


)
target_link_libraries(MessageSaver
 Generics
 PlainStorage3
 PrivacyFilter
 Commons
 LogCommons
)
install(TARGETS MessageSaver  DESTINATION ${INSTALL_DIR})


