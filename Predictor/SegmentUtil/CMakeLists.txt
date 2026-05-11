add_pst(SegmentProfilePST  SegmentProfile.pst ${PST_DIRECTORY}/Predictor/SegmentUtil)


add_executable(SegmentUtil
  Application.cpp
)
target_link_libraries(SegmentUtil
  Generics

  Commons
  SegmentProfilePST
  PlainStorage3


  MessageSaver
  PlainStorage2
  ProfileMap

)


#install(TARGETS SegmentUtil  DESTINATION ${INSTALL_DIR})
