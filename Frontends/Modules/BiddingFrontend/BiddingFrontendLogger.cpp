#include <algorithm>
#include <functional>
#include <utility>

#include <Commons/DelegateTaskGoal.hpp>

#include "BiddingFrontendLogger.hpp"

namespace AdServer::Bidding
{
  namespace
  {
    const char ASPECT[] = "BiddingFrontendLogger";
    const Generics::Time RESCHEDULE_AFTER_ERROR(10);
  }

  BiddingFrontendLogger::BiddingFrontendLogger(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const std::string& log_root,
    const Generics::Time& geo_period,
    unsigned long geo_shards)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(new Generics::Planner(callback)),
      task_runner_(new Generics::TaskRunner(callback, 1))
  {
    const unsigned long geo_shards_count = std::max<unsigned long>(geo_shards, 1);
    geo_loggers_.reserve(geo_shards_count);
    for (unsigned long i = 0; i < geo_shards_count; ++i)
    {
      geo_loggers_.emplace_back(new GeoLogHolder(
        AdServer::LogProcessing::LogFlushTraits(geo_period, log_root.c_str())));
    }

    add_child_object(scheduler_.in());
    add_child_object(task_runner_.in());

    Commons::make_repeating_task(
      std::bind(&BiddingFrontendLogger::flush_logs_, this),
      task_runner_,
      scheduler_)->schedule(geo_period);
  }

  void
  BiddingFrontendLogger::process_geo(const GeoParams& params) noexcept
  {
    if (params.ip.empty())
    {
      return;
    }

    try
    {
      AdServer::LogProcessing::GeoLoggerKey key;
      key.ip.assign(params.ip.data(), params.ip.size());
      key.source.assign(params.source.data(), params.source.size());

      AdServer::LogProcessing::GeoLoggerData data;
      data.lat = params.lat;
      data.lon = params.lon;
      data.type.assign(params.type.data(), params.type.size());
      data.country.assign(params.country.data(), params.country.size());
      data.region.assign(params.region.data(), params.region.size());
      data.city.assign(params.city.data(), params.city.size());

      GeoLogHolder& geo_logger = *geo_loggers_[key.hash() % geo_loggers_.size()];
      geo_logger.add_record(std::move(key), std::move(data));
    }
    catch (const eh::Exception& ex)
    {
      if (logger_)
      {
        Stream::Error ostr;
        ostr << "BiddingFrontendLogger::process_geo(): eh::Exception caught: " <<
          ex.what();
        logger_->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
      }
    }
  }

  Generics::Time
  BiddingFrontendLogger::flush_logs_() noexcept
  {
    try
    {
      const Generics::Time now = Generics::Time::get_time_of_day();
      Generics::Time next_flush;

      for (auto& geo_logger : geo_loggers_)
      {
        const Generics::Time local_next_flush =
          geo_logger->flush_if_required(now);
        if (local_next_flush != Generics::Time::ZERO)
        {
          next_flush = next_flush == Generics::Time::ZERO ?
            local_next_flush :
            std::min(next_flush, local_next_flush);
        }
      }

      return next_flush;
    }
    catch (const eh::Exception& ex)
    {
      if (logger_)
      {
        Stream::Error ostr;
        ostr << "BiddingFrontendLogger::flush_logs_(): eh::Exception caught: " <<
          ex.what();
        logger_->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
      }

      return Generics::Time::get_time_of_day() + RESCHEDULE_AFTER_ERROR;
    }
  }
}
