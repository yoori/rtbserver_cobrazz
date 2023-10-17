add_library(BidCostPredictorLib SHARED
  Application.cpp
  Aggregator.cpp
  AggregatorMultyThread.cpp
  Configuration.cpp
  ContainerImpl.cpp
  CtrCollector.cpp
  DataModelProviderImpl.cpp
  Daemon.cpp
  DaemonImpl.cpp
  LogHelper.cpp
  ModelCtrImpl.cpp
  ModelEvaluatorCtr.cpp
  ModelEvaluatorBidCost.cpp
  ModelBidCostImpl.cpp
  ModelProcessor.cpp
  Process.cpp
  Regenerator.cpp
  Reaggregator.cpp
  ReaggregatorMultyThread.cpp
  Storage.cpp
  BidCostCollector.cpp
  Utils.cpp
)

target_link_libraries(BidCostPredictorLib
  Generics
  Commons
  LogCommons
  CORBACommons
)

add_executable(BidCostPredictor
  Main.cpp
)

target_link_libraries(BidCostPredictor
  BidCostPredictorLib
)

add_executable(BidCostPredictorTest
  Test.cpp
)

target_link_libraries(BidCostPredictorTest
  BidCostPredictorLib
)

install(TARGETS BidCostPredictorLib BidCostPredictor BidCostPredictorTest  DESTINATION ${INSTALL_DIR})