#ifndef RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_FILECLEANER_HPP
#define RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_FILECLEANER_HPP

#include "Utils.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

template<template<class> class Container = std::list>
class FileCleaner
{
public:
  FileCleaner(Container<Utils::GeneratedPath>& files)
              : files_(files)
  {
  }

  ~FileCleaner()
  {
    if (!need_clear_)
      return;

    for (const auto& [temp_path, result_path] : files_)
    {
      std::remove(temp_path.c_str());
      std::remove(result_path.c_str());
    }
  }

  void clearTemp()
  {
    need_clear_ = false;
    for (const auto& [temp_path, result_path] : files_)
    {
      std::remove(temp_path.c_str());
    }
  }

private:
  bool need_clear_ = true;

  Container<Utils::GeneratedPath>& files_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_FILECLEANER_HPP
