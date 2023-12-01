add_executable(PlainStorageUtil
  Application.cpp
)

target_link_libraries(PlainStorageUtil
  Generics
  PlainStorage2
  PlainStorage3
  UServerUtils
  pcre
)

add_executable(LevelCheckUtil
  LevelCheckUtil.cpp
)

target_link_libraries(LevelCheckUtil
  Commons
  Generics
  PlainStorage3
  ProfileMap
)