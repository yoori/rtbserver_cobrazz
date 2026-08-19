#pragma once

#include <string>

#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/MonoAllocator.hpp>
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
      GeoParams();

      GeoParams(const GeoParams&) = delete;
      GeoParams& operator=(const GeoParams&) = delete;
      GeoParams(GeoParams&&) = delete;
      GeoParams& operator=(GeoParams&&) = delete;

      static constexpr std::size_t ARENA_SIZE = 512;

      Generics::MonoAllocatorFixedArena<ARENA_SIZE> arena;
      Generics::MonoString ip;
      Generics::MonoString source;
      AdServer::CampaignSvcs::CoordDecimal lat =
        AdServer::CampaignSvcs::CoordDecimal::ZERO;
      AdServer::CampaignSvcs::CoordDecimal lon =
        AdServer::CampaignSvcs::CoordDecimal::ZERO;
      Generics::MonoString type;
      Generics::MonoString country;
      Generics::MonoString region;
      Generics::MonoString city;
    };

    BiddingFrontendLogger(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const std::string& log_root,
      const Generics::Time& geo_period,
      unsigned long geo_shards);

    void
    process_geo(GeoParams&& params) noexcept;

  protected:
    virtual
    ~BiddingFrontendLogger() noexcept = default;

  private:
    using GeoSavePolicy = AdServer::LogProcessing::SimpleCsvSavePolicy<
      AdServer::LogProcessing::GeoLoggerTraits>;
    using GeoLogHolder = AdServer::LogProcessing::LogHolderSharded<
      AdServer::LogProcessing::GeoLoggerTraits,
      GeoSavePolicy>;
    using GeoLogHolder_var = ReferenceCounting::SmartPtr<GeoLogHolder>;

    Generics::Time
    flush_logs_() noexcept;

  private:
    Logging::Logger_var logger_;
    GeoLogHolder_var geo_logger_;
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;
  };

  using BiddingFrontendLogger_var =
    ReferenceCounting::SmartPtr<BiddingFrontendLogger>;
}
