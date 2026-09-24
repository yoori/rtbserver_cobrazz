add_pst(RequestProfile_v351PST
  RequestProfile_v351.pst
  ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_pst(RequestProfile_v356PST
  RequestProfile_v356.pst
  ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_pst(RequestProfile_v360PST
  RequestProfile_v360.pst
  ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_pst(RequestProfile_v361PST
  RequestProfile_v361.pst
  ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_pst(RequestProfile_v362PST
  RequestProfile_v362.pst
  ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_pst(RequestProfile_v363PST
  RequestProfile_v363.pst
  ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_pst(RequestOperationImpressionProfile_v0PST
  RequestOperationImpressionProfile_v0.pst
  ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_pst(PassbackProfile_v33PST
  PassbackProfile_v33.pst
  ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_library(RequestInfoManagerCompatibility SHARED
  RequestProfileAdapter.cpp
  RequestOperationImpressionProfileAdapter.cpp
)

target_link_libraries(RequestInfoManagerCompatibility
  PRIVATE
    Generics
    RequestProfile_v351PST
    RequestProfile_v356PST
    RequestProfile_v360PST
    RequestProfile_v361PST
    RequestProfile_v362PST
    RequestProfile_v363PST
    RequestOperationImpressionProfile_v0PST
    RequestProfilePST
    RequestOperationProfilePST
)
