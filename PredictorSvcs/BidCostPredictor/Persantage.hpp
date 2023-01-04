#ifndef BIDCOSTPREDICTOR_PERSANTAGE_HPP
#define BIDCOSTPREDICTOR_PERSANTAGE_HPP

// STD
#include <string>

// THIS
#include "Generics/Uncopyable.hpp"
#include "Logger/Logger.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class Persantage final : private Generics::Uncopyable
{
public:
  Persantage(const Logging::Logger_var& logger,
             const char* aspect,
             const std::size_t diff = 5,
             const std::size_t total_number = 0)
             : logger_(logger),
               aspect_(aspect),
               diff_(diff),
               total_number_(total_number)
  {
  }

  ~Persantage() = default;

  void setTotalNumber(const std::size_t total_number)
  {
    total_number_ = total_number;
  }

  void increase()
  {
    if (total_number_ == 0)
    {
      logger_->error(std::string("Total number is null"), aspect_);
      return;
    }

    if (current_number_ * 100 >= counter_ * diff_ * total_number_)
    {
      counter_ += 1;
      const std::size_t percentage =
              (current_number_ * 100) / total_number_;
      logger_->info(
              "Percentage of processed data = " +
              std::to_string(percentage),
              aspect_);
    }
    current_number_ += 1;
  }

private:
  Logging::Logger_var logger_;

  const char* aspect_;

  std::size_t diff_ = 0;

  std::size_t total_number_ = 0;

  std::size_t current_number_ = 0;

  std::size_t counter_ = 0;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif // BIDCOSTPREDICTOR_PERSANTAGE_HPP
