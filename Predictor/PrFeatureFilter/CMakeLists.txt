
add_executable(PrFeatureFilter2
Application.cpp
)
target_link_libraries(PrFeatureFilter2
 Generics

 Commons
)

install(TARGETS PrFeatureFilter2 DESTINATION ${INSTALL_DIR})

