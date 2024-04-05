add_library(CommonModule SHARED
  CommonModule.cpp
)

target_link_libraries(CommonModule
  AdServerCommonsXSD
  BillStatInfoIDL
  CampaignServerIDL
  Commons
  DomainConfigXSD
  DomainParser
  FeConfigXSD
  GenericSegmentor
  Generics
  Polyglot
  RequestMatchers
  StatInfoIDL
  UserIdController
)