add_executable(PlainStorageUtil
 Application.cpp
)

target_link_libraries(PlainStorageUtil
 Generics
 PlainStorage2
 PlainStorage3
pcre
)


add_executable(LevelCheckUtil
 LevelCheckUtil.cpp

)
target_link_libraries(LevelCheckUtil

 Generics
 Commons
 PlainStorage3
)
#install(TARGETS PlainStorageUtil LevelCheckUtil  DESTINATION ${INSTALL_DIR})