#pragma once

#include <string_view>
#include <vector>

#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <LogCommons/GeoLogger.hpp>
#include <LogCommons/LogHolder.hpp>

namespace AdServer::Bidding
{
  class BiddingFrontendLogger:
    public Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct GeoParams
    {
      std::string_view ip;
      std::string_view source;
      AdServer::CampaignSvcs::CoordDecimal lat =
        AdServer::CampaignSvcs::CoordDecimal::ZERO;
      AdServer::CampaignSvcs::CoordDecimal lon =
        AdServer::CampaignSvcs::CoordDecimal::ZERO;
      std::string_view type;
      std::string_view country;
      std::string_view region;
      std::string_view city;
    };

    BiddingFrontendLogger(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const std::string& log_root,
      const Generics::Time& geo_period,
      unsigned long geo_shards);

    void
    process_geo(const GeoParams& params) noexcept;

  protected:
    virtual
    ~BiddingFrontendLogger() noexcept = default;

  private:
    using GeoLogHolder = AdServer::LogProcessing::LogHolderPoolData<
      AdServer::LogProcessing::GeoLoggerTraits,
      AdServer::LogProcessing::SimpleCsvSavePolicy<
        AdServer::LogProcessing::GeoLoggerTraits>>;
    using GeoLogHolder_var = ReferenceCounting::SmartPtr<GeoLogHolder>;
    using GeoLogHolderArray = std::vector<GeoLogHolder_var>;

    Generics::Time
    flush_logs_() noexcept;

  private:
    Logging::Logger_var logger_;
    GeoLogHolderArray geo_loggers_;
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;
  };

  using BiddingFrontendLogger_var =
    ReferenceCounting::SmartPtr<BiddingFrontendLogger>;
}
