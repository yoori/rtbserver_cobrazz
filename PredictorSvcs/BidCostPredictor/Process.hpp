#ifndef BIDCOSTPREDICTOR_PROCESS_H
#define BIDCOSTPREDICTOR_PROCESS_H

// STD
#include <mutex>
#include <string>
#include <vector>

// POSIX
#include <unistd.h>

// THIS
#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class Process final
  : public virtual ReferenceCounting::AtomicImpl
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
public:
  explicit Process(
    const std::string& bin_path,
    const std::vector<std::string>& options);

  ~Process() override;

  void launch();

  int wait() noexcept;

  void stop() noexcept;

private:
  const std::string bin_path_;

  const std::vector<std::string> options_;

  std::mutex mutex_;

  pid_t pid_ = -1;

  bool is_stopped_ = false;
};

using Process_var = ReferenceCounting::SmartPtr<Process>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_PROCESS_H
