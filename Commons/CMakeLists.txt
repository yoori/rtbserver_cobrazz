include(CheckCXXCompilerFlag)

add_idl(CampaignFreqIDL CampaignFreq.idl ${IDL_DIRECTORY}/Commons NO_SERVANT)
add_idl(CookieInfoPackIDL CookieInfoPack.idl ${IDL_DIRECTORY}/Commons NO_SERVANT)
add_idl(CorbaTypesIDL CorbaTypes.idl ${IDL_DIRECTORY}/Commons NO_SERVANT)
add_idl(FreqCapInfoIDL FreqCapInfo.idl ${IDL_DIRECTORY}/Commons NO_SERVANT)

target_link_libraries(FreqCapInfoIDL
  PUBLIC
    CorbaTypesIDL
)

add_library(Commons SHARED
  SignalActiveObject.cpp
  HostDistribution.cpp
  Hostname.cpp
  Hostname.hpp
  PathManip.cpp
  PidFileGuard.cpp
  ActivityGate.cpp
  AsyncMutex.cpp
  OrderedAsyncTaskWindow.cpp
  CountActiveObject.cpp
  ZmqConfig.cpp
  IPCrypter.cpp
  UserInfoManip.cpp
  Algs.cpp
  CsvReader.cpp
  ExternalUserIdUtils.cpp
  BoostAsioContextRunActiveObject.cpp
  ExecutorPool.cpp
  FastScheduler.cpp
  FastScheduler.hpp
  ZmqSocketHolder.cpp
  SecToken.cpp
  FastJsonParser.cpp
)

target_link_libraries(Commons
  PUBLIC
    Generics
    AdServerCommonsXSD
    HTTP
    OpenSSL::Crypto
    PkgConfig::ZeroMQ
    Sync
    Boost::headers
  PRIVATE
    HostDistributionFileXSD
    Threads::Threads
)

add_library(CORBAConfig SHARED
  CorbaConfig.cpp
)

target_link_libraries(CORBAConfig
  PRIVATE
    Commons
    CORBACommons
    AdServerCommonsXSD
)

set(fast_json_parser_x86 OFF)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86)$")
  set(fast_json_parser_x86 ON)
endif()

if (fast_json_parser_x86)
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

  if (FAST_JSON_PARSER_CAN_COMPILE_SSE2)
    target_sources(Commons PRIVATE
      FastJsonParserSimdSse2.cpp
    )
    set_source_files_properties(
      FastJsonParserSimdSse2.cpp
      PROPERTIES COMPILE_OPTIONS "-msse2")
    target_compile_definitions(Commons PRIVATE
      AD_FAST_JSON_PARSER_HAS_SSE2)
  endif()

  if (FAST_JSON_PARSER_CAN_COMPILE_SSE2 AND
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

  if (FAST_JSON_PARSER_CAN_COMPILE_SSE2 AND
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
  PRIVATE
    CORBACommons
    Logger
)

add_subdirectory(Kafka)
add_subdirectory(HttpServer)
#add_subdirectory(Oracle)
add_subdirectory(Postgres)
add_subdirectory(Xslt)
