
add_executable(PrFeatureFilter2
  Application.cpp
)
target_link_libraries(PrFeatureFilter2
  PRIVATE
    Generics

)

#install(TARGETS PrFeatureFilter2 DESTINATION ${INSTALL_DIR})
