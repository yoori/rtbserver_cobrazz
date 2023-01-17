add_library(CommonModule SHARED
CommonModule.cpp
)
target_link_libraries(CommonModule
 Generics

 Commons
 StatInfoIDL BillStatInfoIDL CampaignServerIDL
 FeConfigXSD
AdServerCommonsXSD
 RequestMatchers
 DomainParser
 Polyglot
 UserIdController
DomainConfigXSD
GenericSegmentor
)

install(TARGETS CommonModule DESTINATION ${INSTALL_DIR})