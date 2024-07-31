#ifndef BIDCOSTPREDICTOR_PROCESS_H
#define BIDCOSTPREDICTOR_PROCESS_H

// POSIX
#include <unistd.h>

// STD
#include <mutex>
#include <string>
#include <vector>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class Process final : public virtual ReferenceCounting::AtomicImpl
{
public:
  using Option = std::string;
  using Options = std::vector<std::string>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Process(
    const std::string& bin_path,
    const Options& options);

  void launch();

  int wait() noexcept;

  void stop() noexcept;

protected:
  ~Process() override = default;

private:
  const std::string bin_path_;

  const Options options_;

  std::mutex mutex_;

  pid_t pid_ = -1;
};

using Process_var = ReferenceCounting::SmartPtr<Process>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_PROCESS_H
