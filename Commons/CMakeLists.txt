
include(CheckCXXCompilerFlag)

add_idl(CampaignFreqIDL CampaignFreq.idl ${IDL_DIRECTORY}/Commons)
add_idl(CookieInfoPackIDL CookieInfoPack.idl ${IDL_DIRECTORY}/Commons)
add_idl(CorbaTypesIDL CorbaTypes.idl ${IDL_DIRECTORY}/Commons)
add_idl(FreqCapInfoIDL FreqCapInfo.idl ${IDL_DIRECTORY}/Commons)

target_link_libraries(CampaignFreqIDL CorbaTypesIDL)
target_link_libraries(CookieInfoPackIDL CorbaTypesIDL)
target_link_libraries(FreqCapInfoIDL CorbaTypesIDL)



add_library(Commons  SHARED
  CorbaConfig.cpp
  SignalActiveObject.cpp
  HostDistribution.cpp
  PathManip.cpp
  PidFileGuard.cpp
  ActivityGate.cpp
  AsyncMutex.cpp
  CountActiveObject.cpp
  ZmqConfig.cpp
  IPCrypter.cpp
  UserInfoManip.cpp
  Algs.cpp
  CsvReader.cpp
  ExternalUserIdUtils.cpp
  BoostAsioContextRunActiveObject.cpp
  ExecutorPool.cpp
  ZmqSocketHolder.cpp
  SecToken.cpp
  FastJsonParser.cpp

)

target_link_libraries(Commons
  ProcessControlIDL
  HostDistributionFileXSD xerces-c Generics AdServerCommonsXSD HTTP CORBACommons crypto
  TAO TAO_AnyTypeCode  TAO_CodecFactory TAO_CosEvent TAO_CosNaming TAO_CosNotification TAO_DynamicAny TAO_EndpointPolicy TAO_FaultTolerance
  TAO_FT_ClientORB TAO_FT_ServerORB TAO_FTORB_Utils TAO_IORManip TAO_IORTable
  TAO_Messaging TAO_PI TAO_PI_Server TAO_PortableGroup TAO_PortableServer TAO_Security TAO_SSLIOP TAO_TC TAO_TC_IIOP TAO_Valuetype ACE_SSL
  zmq  pthread ACE Logger Sync
  CorbaObjectRefIDL
)

set(fast_json_parser_x86 OFF)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86)$")
  set(fast_json_parser_x86 ON)
endif()

if(fast_json_parser_x86)
  check_cxx_compiler_flag(
    "-msse2"
    FAST_JSON_PARSER_CAN_COMPILE_SSE2)
  check_cxx_compiler_flag(
    "-mavx2"
    FAST_JSON_PARSER_CAN_COMPILE_AVX2)
  check_cxx_compiler_flag(
    "-mavx512f"
    FAST_JSON_PARSER_CAN_COMPILE_AVX512F)
  check_cxx_compiler_flag(
    "-mavx512bw"
    FAST_JSON_PARSER_CAN_COMPILE_AVX512BW)

  if(FAST_JSON_PARSER_CAN_COMPILE_SSE2)
    target_sources(Commons PRIVATE
      FastJsonParserSimdSse2.cpp
    )
    set_source_files_properties(
      FastJsonParserSimdSse2.cpp
      PROPERTIES COMPILE_OPTIONS "-msse2")
    target_compile_definitions(Commons PRIVATE
      AD_FAST_JSON_PARSER_HAS_SSE2)
  endif()

  if(FAST_JSON_PARSER_CAN_COMPILE_SSE2 AND
    FAST_JSON_PARSER_CAN_COMPILE_AVX2)
    target_sources(Commons PRIVATE
      FastJsonParserSimdAvx2.cpp
    )
    set_source_files_properties(
      FastJsonParserSimdAvx2.cpp
      PROPERTIES COMPILE_OPTIONS "-mavx2")
    target_compile_definitions(Commons PRIVATE
      AD_FAST_JSON_PARSER_HAS_AVX2)
  endif()

  if(FAST_JSON_PARSER_CAN_COMPILE_SSE2 AND
    FAST_JSON_PARSER_CAN_COMPILE_AVX2 AND
    FAST_JSON_PARSER_CAN_COMPILE_AVX512F AND
    FAST_JSON_PARSER_CAN_COMPILE_AVX512BW)
    target_sources(Commons PRIVATE
      FastJsonParserSimdAvx512.cpp
    )
    set_source_files_properties(
      FastJsonParserSimdAvx512.cpp
      PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512bw")
    target_compile_definitions(Commons PRIVATE
      AD_FAST_JSON_PARSER_HAS_AVX512BW)
  endif()
endif()

add_library(ProcessControlVars SHARED
  ProcessControlVarsImpl.cpp
)

target_link_libraries(ProcessControlVars
  Stream
  CORBACommons
  AdServerCommonsXSD
  Generics
  CorbaTypesIDL
  Commons
  zmq  pthread ACE Logger Sync
)





ADD_SUBDIRECTORY(Kafka)
ADD_SUBDIRECTORY(HttpServer)
#ADD_SUBDIRECTORY(Oracle)
ADD_SUBDIRECTORY(Postgres)
ADD_SUBDIRECTORY(Xslt)


#install(TARGETS CampaignFreqIDL CookieInfoPackIDL CorbaTypesIDL FreqCapInfoIDL Commons ProcessControlVars   DESTINATION ${INSTALL_DIR})
