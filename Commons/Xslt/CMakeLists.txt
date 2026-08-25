include_directories(${LIBXML2_INCLUDE_DIRS})


add_executable(XslTransformAdmin
  XslTransformAdmin.cpp
)
target_link_libraries(XslTransformAdmin
  PRIVATE
    XslTransformer
    Generics
    pthread
    xml2
    xslt
)

add_library(XslTransformer STATIC
  LibxsltTransformer.cpp
  XslTransformer.cpp

)

target_link_libraries(XslTransformer
  PRIVATE
    Generics
    pthread
    xml2
    xslt
)

add_library(XsltFileCache STATIC
  XsltFileCache.cpp
)

target_link_libraries(XsltFileCache
  PRIVATE
    Generics
    Sync
    XslTransformer
)

#install(TARGETS XslTransformAdmin  DESTINATION ${INSTALL_DIR})
