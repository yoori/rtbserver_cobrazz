#ifndef BIDCOSTPREDICTOR_PERSANTAGE_HPP
#define BIDCOSTPREDICTOR_PERSANTAGE_HPP

// STD
#include <string>

// UNIXCOMMONS
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>

// THIS
#include "Utils.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class Persantage final : private Generics::Uncopyable
{
public:
  explicit Persantage(
    Logging::Logger* logger,
    const std::string& aspect,
    const std::size_t diff = 5,
    const std::size_t total_number = 0,
    const bool need_print_memory = true)
    : logger_(ReferenceCounting::add_ref(logger)),
      aspect_(aspect),
      diff_(diff),
      total_number_(total_number),
      need_print_memory_(need_print_memory)
  {
  }

  ~Persantage() = default;

  void set_total_number(const std::size_t total_number)
  {
    total_number_ = total_number;
  }

  void increase() noexcept
  {
    try
    {
      if (total_number_ == 0)
      {
        Stream::Error stream;
        stream << FNS
               << "Total number is null";
        logger_->error(stream.str(), aspect_.c_str());

        return;
      }

      if (current_number_ * 100 >= counter_ * diff_ * total_number_)
      {
        counter_ += 1;
        const std::size_t percentage = (current_number_ * 100) / total_number_;

        std::ostringstream stream;
        stream << FNS
               << "Percentage of processed data = "
               << std::to_string(percentage);
        logger_->info(stream.str(), aspect_.c_str());

        if (need_print_memory_)
        {
          const auto memory = Utils::memory_process_usage();
          std::ostringstream stream;
          stream << FNS
                 << "Ram[Gb] = "
                 << memory.second;
          logger_->info(stream.str(), aspect_.c_str());
        }
      }
      current_number_ += 1;
    }
    catch (...)
    {
    }
  }

private:
  const Logging::Logger_var logger_;

  const std::string aspect_;

  const std::size_t diff_ = 0;

  std::size_t total_number_ = 0;

  std::size_t current_number_ = 0;

  std::size_t counter_ = 0;

  bool need_print_memory_ = false;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif // BIDCOSTPREDICTOR_PERSANTAGE_HPP