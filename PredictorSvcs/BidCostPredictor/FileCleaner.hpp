#ifndef BIDCOSTPREDICTOR_FILECLEANER_HPP
#define BIDCOSTPREDICTOR_FILECLEANER_HPP

#include "Utils.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class FileCleaner final
{
public:
  using Files = std::list<Utils::GeneratedPath>;

public:
  explicit FileCleaner(Files& files)
    : files_(files)
  {
  }

  ~FileCleaner()
  {
    if (is_temp_files_removed_)
    {
      return;
    }

    for (const auto& [temp_path, result_path] : files_)
    {
      std::remove(temp_path.c_str());
      std::remove(result_path.c_str());
    }
  }

  void remove_temp_files() noexcept
  {
    is_temp_files_removed_ = true;
    for (const auto& [temp_path, result_path] : files_)
    {
      std::remove(temp_path.c_str());
    }
  }

private:
  bool is_temp_files_removed_ = false;

  Files& files_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_FILECLEANER_HPP
