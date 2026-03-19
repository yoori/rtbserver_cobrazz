add_library(CommonModule SHARED
  CommonModule.cpp
)

target_link_libraries(CommonModule
  StatInfoIDL
  BillStatInfoIDL
  CampaignServerIDL

  AdServerCommonsXSD

  Commons
  RequestMatchers
  DomainParser
  Polyglot
  UserIdController
  GenericSegmentor

  FeConfigXSD
  DomainConfigXSD

  Generics
)
