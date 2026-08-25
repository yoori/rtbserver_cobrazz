

add_executable(DNSDecodeUtil
  Application.cpp
)
target_link_libraries(DNSDecodeUtil
  PRIVATE
    Commons

)

#install(TARGETS DNSDecodeUtil DESTINATION ${INSTALL_DIR})
