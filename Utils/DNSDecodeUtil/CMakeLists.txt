

add_executable(DNSDecodeUtil
 Application.cpp
)
target_link_libraries(DNSDecodeUtil
 Generics
 String
 Stream
 Commons

)

install(TARGETS DNSDecodeUtil DESTINATION ${INSTALL_DIR})