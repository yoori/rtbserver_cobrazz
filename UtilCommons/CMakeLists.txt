add_library(UtilCommons SHARED
  Table.cpp
)
target_link_libraries(UtilCommons
  PRIVATE
    #String
    Generics
)
