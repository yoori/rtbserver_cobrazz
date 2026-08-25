add_executable(XslTransformAdmin
  XslTransformAdmin.cpp
)
target_link_libraries(XslTransformAdmin
  PRIVATE
    XslTransformer
    Generics
    Threads::Threads
    LibXml2::LibXml2
    LibXslt::LibXslt
)

add_library(XslTransformer STATIC
  LibxsltTransformer.cpp
  XslTransformer.cpp

)

target_link_libraries(XslTransformer
  PRIVATE
    Generics
    Threads::Threads
    LibXml2::LibXml2
    LibXslt::LibXslt
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
