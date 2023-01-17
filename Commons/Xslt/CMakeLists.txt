include_directories(${LIBXML2_INCLUDE_DIRS})


add_executable(XslTransformAdmin 
    XslTransformAdmin.cpp
)
target_link_libraries(XslTransformAdmin
XslTransformer Generics pthread
)

add_library(XslTransformer STATIC
    LibxsltTransformer.cpp 
    XslTransformer.cpp

)

target_link_libraries(XslTransformer
 xml2
 xslt
 HTTP

)

add_library(XsltFileCache STATIC
XsltFileCache.cpp
)

target_link_libraries(XsltFileCache String Generics AdServerCommonsXSD Sync XslTransformer
)

install(TARGETS XslTransformAdmin  DESTINATION ${INSTALL_DIR})