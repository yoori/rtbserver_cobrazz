add_executable(PrFeatureFilter
  Application.cpp
)

target_link_libraries(PrFeatureFilter
  PRIVATE
    Generics
)
#install(TARGETS PrFeatureFilter  DESTINATION ${INSTALL_DIR})

