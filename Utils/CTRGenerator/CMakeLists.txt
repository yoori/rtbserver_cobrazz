include_directories(/opt/foros/vanga/include)

add_library(CTRGeneratorLib SHARED
  CalculateParamsFilter.cpp 
  CTRGenerator.cpp
)

target_link_libraries(CTRGeneratorLib 
  Generics
  CTRGeneratorConfigXSD
  CTRGeneratorDataConfigXSD
  SVMGeneratorConfigXSD
  CTRProvider
)

add_executable(CTRGenerator
  Application.cpp
)

target_link_libraries(CTRGenerator
  CTRGeneratorLib
)
