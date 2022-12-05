#ifndef _EXECUTION_TIME_TRACER_HPP_
#define _EXECUTION_TIME_TRACER_HPP_

#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

namespace AdServer
{
namespace CampaignSvcs
{
  class ExecutionTimeTracer
  {
  public:
    ExecutionTimeTracer(const char* fun,
      const char* aspect,
      Logging::Logger* logger,
      const char* operation = "")
      : fun_(fun),
        aspect_(aspect),
        operation_(operation[0] ? (std::string(operation) + " ") :
          std::string()),
        logger_(ReferenceCounting::add_ref(logger))
    {
      timer.start();
    }

    virtual ~ExecutionTimeTracer() noexcept
    {
      timer.stop();

      logger_->sstream(Logging::Logger::DEBUG,
        aspect_.c_str()) << fun_ <<
        ": " << operation_ << "execution time: " << timer.elapsed_time();
    }
    
  private:
    std::string fun_;
    std::string aspect_;
    std::string operation_;
    Logging::Logger_var logger_;
    Generics::Timer timer;
  };
}
}

#endif
