add_pst(SegmentProfilePST  SegmentProfile.pst ${PST_DIRECTORY}/Predictor/SegmentUtil)


add_executable(SegmentUtil
Application.cpp
)
target_link_libraries(SegmentUtil
 Generics

 Commons
# IPMap
#Geoip
SegmentProfilePST
 PlainStorage3


# ProfilingCommons
MessageSaver
PlainStorage2
PlainStorage3
#PlainStorageUtil
ProfileMap

)


install(TARGETS SegmentUtil  DESTINATION ${INSTALL_DIR})
