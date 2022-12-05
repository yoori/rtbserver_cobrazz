#ifndef AD_SERVER_LOG_PROCESSING_LOG_TYPE_EXT_TRAITS_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_TYPE_EXT_TRAITS_HPP


#include <LogCommons/LogCommons.hpp>
#include <LogCommons/FileReceiver.hpp>
#include <LogCommons/ActionRequest.hpp>
#include <LogCommons/ActionStat.hpp>
#include <LogCommons/CampaignUserStat.hpp>
#include <LogCommons/CcUserStat.hpp>
#include <LogCommons/CcgKeywordStat.hpp>
#include <LogCommons/CcgSelectionFailureStat.hpp>
#include <LogCommons/CcgStat.hpp>
#include <LogCommons/CcgUserStat.hpp>
#include <LogCommons/ChannelCountStat.hpp>
#include <LogCommons/ChannelHitStat.hpp>
#include <LogCommons/ChannelImpInventory.hpp>
#include <LogCommons/ChannelInventory.hpp>
#include <LogCommons/ChannelInventoryEstimationStat.hpp>
#include <LogCommons/ChannelOverlapUserStat.hpp>
#include <LogCommons/ChannelPerformance.hpp>
#include <LogCommons/ChannelPriceRange.hpp>
#include <LogCommons/ChannelTriggerImpStat.hpp>
#include <LogCommons/ChannelTriggerStat.hpp>
#include <LogCommons/CmpStat.hpp>
#include <LogCommons/ColoUpdateStat.hpp>
#include <LogCommons/ColoUserStat.hpp>
#include <LogCommons/ColoUsers.hpp>
#include <LogCommons/CreativeStat.hpp>
#include <LogCommons/DeviceChannelCountStat.hpp>
#include <LogCommons/ExpressionPerformance.hpp>
#include <LogCommons/PageLoadsDailyStat.hpp>
#include <LogCommons/PassbackStat.hpp>
#include <LogCommons/SearchEngineStat.hpp>
#include <LogCommons/SearchTermStat.hpp>
#include <LogCommons/SiteReferrerStat.hpp>
#include <LogCommons/SiteUserStat.hpp>
#include <LogCommons/TagAuctionStat.hpp>
#include <LogCommons/TagPositionStat.hpp>
#include <LogCommons/UserAgentStat.hpp>
#include <LogCommons/UserProperties.hpp>
#include <LogCommons/WebStat.hpp>
#include <LogCommons/CampaignReferrerStat.hpp>
#include <LogCommons/GenericLogCsvSaverImpl.hpp>
#include "LogTypeCsvTraits.hpp"
#include "ThreadLogSaverImpl.hpp"

namespace AdServer
{
namespace LogProcessing
{
  enum LOG_CONV_RESULT { LCR_NIL = -1, LCR_OK, LCR_PARTIAL };

  extern const char DEFERRED_DIR[];

  namespace Aux_
  {
    struct NoConvertion
    {
      template <typename LogVersionManager>
      static void
      add_conversion_support(LogVersionManager*) noexcept
      {}
    };

    struct RejectFiles
    {
      static void
      reject_files(
        const CollectorBundleFileList& file_list,
        const std::string& in_dir,
        const char* error_folder_name,
        const Generics::Time& /*save_time*/)
      {
        FileStore(in_dir, error_folder_name).store(file_list);
      }
    };

    struct RejectDeferredFiles
    {
      static void
      reject_files(
        const CollectorBundleFileList& file_list,
        const std::string& in_dir,
        const char* error_folder_name,
        const Generics::Time& save_time)
      {
        if (error_folder_name)
        {
          FileStore(in_dir, error_folder_name).store(file_list);
        }
        else
        {
          FileStore fs(in_dir, DEFERRED_DIR, false);
          fs.store(file_list, save_time == Generics::Time::ZERO ?
                   Generics::Time::get_time_of_day() : save_time);
        }
      }
    };

    template <
      class LogTraits,
      class CONVERSION_ = NoConvertion,
      typename FilesRejector = RejectFiles>
    struct BaseLogTraitsHelper :
      public LogTraits, public CONVERSION_, public FilesRejector
    {
      typedef GenericLogLoaderImpl<
        BaseLogTraitsHelper,
        typename Detail::BundleLoadPolicySelector<
          BaseLogTraitsHelper, LogTraits::is_nested>::LoadType
        > ThreadSafeLoaderType;

      typedef typename Detail::SaverImplSelector<
        BaseLogTraitsHelper,
        true,
        LogTraits::is_nested>::Type
        ThreadSafeSaverType;
 
      typedef typename Detail::DistribSaverImplSelector<
        BaseLogTraitsHelper,
        true,
        LogTraits::is_nested>::Type
        DistribThreadSafeSaverType;

      typedef GenericLogCsvSaverImpl<LogTraits> CsvSaverType;

      template <typename CompatLoaderT>
      struct CompatLoaderSelector
      {
        typedef CompatLoaderT Type;
      };

      template <typename ConvTraits>
      struct ConvTraitsSelector
      {
        typedef ConvTraits Type;
      };

      static
      LOG_CONV_RESULT
      make_conversions(
        std::istream&,
        const CollectorBundleFileGuard_var&,
        const std::string&)
        noexcept
      {
        return LCR_NIL;
      }

      /**
       * @return True if need conversion for logs files of another type to this log type
       */
      static
      bool
      other_logs_conversion(const CollectorBundleFileGuard_var& /*file_handle*/)
      {
        return false;
      }

      static
      void
      search_files(
        const std::string& in_dir,
        const FileReceiver_var& file_receiver)
      {
        file_receiver->fetch_files(in_dir.c_str(), LogTraits::log_base_name());
      }
    };

    template <typename FromLogTraits, typename ToLogTraits>
    struct ConvertAllBelow_2_6: BaseLogTraitsHelper<FromLogTraits>
    {
      static
      LOG_CONV_RESULT
      make_conversions(
        std::istream& is,
        const CollectorBundleFileGuard_var& file_handle,
        const std::string& version)
        /*throw(eh::Exception)*/;
    };

    template <typename FromLogTraits, typename ToLogTraits>
    struct OtherTypeConversion: BaseLogTraitsHelper<ToLogTraits>
    {
      typedef FromLogTraits ConvTraits;

      template <typename LocalConvTraits>
      struct ConvTraitsSelector
      {
        typedef ConvTraits Type;
      };

      static
      bool
      other_logs_conversion(const CollectorBundleFileGuard_var& file_handle)
        /*throw(eh::Exception)*/
      {
        std::string conv_dir_name = "/";
        conv_dir_name += ConvTraits::log_base_name();
        return file_handle->full_path().find(conv_dir_name) != std::string::npos;
      }
    };

    template <typename FromLogTraits, typename ToLogTraits>
    struct ConversionToOtherType: FromLogTraits
    {
      static
      LOG_CONV_RESULT
      make_conversions(
        std::istream&,
        const CollectorBundleFileGuard_var&,
        const std::string&)
        /*throw(eh::Exception)*/;
    };

    struct CcgCcStatToCcgCcUserStatConversion
    {
      template <typename LogVersionManager>
      static void
      add_conversion_support(LogVersionManager* log_version_manager)
        /*throw(eh::Exception)*/;
    };

    // Add Colo and GlobalColo bundles to single convertion loader
    struct ColoUsers_To_Colo_Or_GlobalColo_UserStatConvertion
    {
      template <typename LogVersionManager>
      static void
      add_conversion_support(LogVersionManager* log_version_manager)
        /*throw(eh::Exception)*/;
    };
  } // namespace Aux_

  typedef Aux_::BaseLogTraitsHelper<ActionRequestCsvTraits>
    ActionRequestExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ActionStatCsvTraits>
    ActionStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<CcgKeywordStatCsvTraits>
    CcgKeywordStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<CcgSelectionFailureStatCsvTraits>
    CcgSelectionFailureStatExtTraits;

  typedef Aux_::ConvertAllBelow_2_6<CcgStatCsvTraits, CcgUserStatTraits>
    CcgStatExtTraits;

  struct CcgUserStatAddConvTraits : CcgUserStatCsvTraits
  {
    typedef CcgStatExtTraits ConvTraits;
  };

  struct CcgUserStatExtTraits: public Aux_::BaseLogTraitsHelper<
    CcgUserStatAddConvTraits,
    Aux_::CcgCcStatToCcgCcUserStatConversion>
  {
    template <typename OldCollector>
    static CcgStatToCcgUserStatLoader<OldCollector>*
    make_conversion_loader(const CollectorBundlePtrType& bundle)
      /*throw(eh::Exception)*/
    {
      return new CcgStatToCcgUserStatLoader<OldCollector>(bundle);
    }
  };

  typedef Aux_::ConvertAllBelow_2_6<CcStatCsvTraits, CcUserStatTraits>
    CcStatExtTraits;

  struct CcUserStatAddConvTraits : CcUserStatCsvTraits
  {
    typedef CcStatExtTraits ConvTraits;
  };

  struct CcUserStatExtTraits: public Aux_::BaseLogTraitsHelper<
    CcUserStatAddConvTraits,
    Aux_::CcgCcStatToCcgCcUserStatConversion>
  {
    template <typename OldCollector>
    static CcStatToCcUserStatLoader<OldCollector>*
    make_conversion_loader(const CollectorBundlePtrType& bundle)
      /*throw(eh::Exception)*/
    {
      return new CcStatToCcUserStatLoader<OldCollector>(bundle);
    }
  };

  typedef Aux_::ConversionToOtherType<CampaignStatTraits, CampaignUserStatTraits>
    CampaignStatExtTraits;

  struct CampaignUserStatExtTraits:
    // Convert CampaignStat (1.0, 1.1) -> CampaignUserStat
    public Aux_::OtherTypeConversion<
      CampaignStatExtTraits,
      CampaignUserStatCsvTraits>
  {
    static
    void
    search_files(
      const std::string& in_dir,
      const FileReceiver_var& file_receiver)
    {
      static std::string conv_dir =
        in_dir.substr(0, in_dir.size() - strlen(log_base_name())) +
        CampaignStatTraits::log_base_name();

      file_receiver->fetch_files(
        in_dir.c_str(),
        CampaignUserStatTraits::log_base_name());

      file_receiver->fetch_files(
        conv_dir.c_str(),
        CampaignStatTraits::log_base_name());
    }

    template <typename Collector>
    static CampaignStatToCampaignUserStatLoader<Collector>*
    make_conversion_loader(const CollectorBundlePtrType& bundle)
      /*throw(eh::Exception)*/
    {
      return new CampaignStatToCampaignUserStatLoader<Collector>(bundle);
    }

    template <typename LogVersionManager>
    static void
    add_conversion_support(LogVersionManager* log_version_manager)
      /*throw(eh::Exception)*/;
  };

  struct AdvertiserUserStatExtTraits:
    public Aux_::BaseLogTraitsHelper<AdvertiserUserStatCsvTraits>
  {
    template <typename CompatLoaderT>
    struct CompatLoaderSelector
    {
      typedef AdvertiserUserStat_V_1_0_To_CurrentLoader Type;
    };
  };

  typedef Aux_::BaseLogTraitsHelper<ChannelInventoryCsvTraits>
    ChannelInventoryExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ChannelImpInventoryCsvTraits>
    ChannelImpInventoryExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ChannelPerformanceCsvTraits>
    ChannelPerformanceExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ChannelPriceRangeCsvTraits>
    ChannelPriceRangeExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ChannelOverlapUserStatCsvTraits>
    ChannelOverlapUserStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ChannelTriggerImpStatCsvTraits>
    ChannelTriggerImpStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ChannelTriggerStatCsvTraits>
    ChannelTriggerStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<DeviceChannelCountStatCsvTraits>
    DeviceChannelCountStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<SearchEngineStatCsvTraits>
    SearchEngineStatExtTraits;

  struct SearchTermStatExtTraits:
    public Aux_::BaseLogTraitsHelper<SearchTermStatCsvTraits>
  {
    typedef HitsFilteringLogLoaderImpl<SearchTermStatExtTraits>
      HitsFilteringLoaderType;
  };

  typedef Aux_::BaseLogTraitsHelper<TagPositionStatCsvTraits>
    TagPositionStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<UserAgentStatCsvTraits>
    UserAgentStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ChannelHitStatCsvTraits>
    ChannelHitStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ColoUpdateStatCsvTraits>
    ColoUpdateStatExtTraits;

  struct ColoUsersExtTraits:
    public Aux_::BaseLogTraitsHelper<ColoUsersTraits>
  {
    static
    LOG_CONV_RESULT
    make_conversions(
      std::istream&,
      const CollectorBundleFileGuard_var&,
      const std::string&)
      /*throw(eh::Exception)*/;
  };

  struct ColoUserStatExtTraits: public Aux_::BaseLogTraitsHelper<
    ColoUserStatCsvTraits,
    Aux_::ColoUsers_To_Colo_Or_GlobalColo_UserStatConvertion>
  {
    typedef ColoUsersExtTraits ConvTraits;
  };

  struct GlobalColoUserStatExtTraits: public Aux_::BaseLogTraitsHelper<
    GlobalColoUserStatCsvTraits,
    Aux_::ColoUsers_To_Colo_Or_GlobalColo_UserStatConvertion>
  {
    typedef ColoUsersExtTraits ConvTraits;
  };

  typedef Aux_::BaseLogTraitsHelper<CreativeStatTraits> CreativeStatExtTraits;

  class CreativeStatDbSaver;
  class CreativeStatPgCsvSaver;
  class DeferredCreativeStatDbSaver;
  class DeferredCreativeStatPgCsvSaver;
  class CmpStatDbSaver;
  class CmpStatPgCsvSaver;
  class DeferredCmpStatDbSaver;
  class DeferredCmpStatPgCsvSaver;

  struct CustomCreativeStatExtTraits: public Aux_::BaseLogTraitsHelper<
    CreativeStatCsvTraits,
    Aux_::NoConvertion,
    Aux_::RejectDeferredFiles>
  {
    typedef CreativeStatDbSaver DbSaverType;
    typedef CreativeStatPgCsvSaver PgCsvSaverType;
  };

  struct DeferredCreativeStatExtTraits:
    public Aux_::BaseLogTraitsHelper<CreativeStatTraits>
  {
    typedef DeferredCreativeStatDbSaver DbSaverType;
    typedef DeferredCreativeStatPgCsvSaver PgCsvSaverType;
  };

  typedef Aux_::BaseLogTraitsHelper<ExpressionPerformanceCsvTraits>
    ExpressionPerformanceExtTraits;

  typedef Aux_::BaseLogTraitsHelper<SiteReferrerStatCsvTraits>
    SiteReferrerStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<UserPropertiesCsvTraits>
    UserPropertiesExtTraits;

  typedef Aux_::BaseLogTraitsHelper<PassbackStatCsvTraits>
    PassbackStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ChannelInventoryEstimationStatCsvTraits>
    ChannelInventoryEstimationStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<CmpStatCsvTraits>
    CmpStatExtTraits;

  struct CustomCmpStatExtTraits: public Aux_::BaseLogTraitsHelper<
    CmpStatCsvTraits,
    Aux_::NoConvertion,
    Aux_::RejectDeferredFiles>
  {
    typedef CmpStatDbSaver DbSaverType;
    typedef CmpStatPgCsvSaver PgCsvSaverType;
  };

  struct DeferredCmpStatExtTraits:
    public Aux_::BaseLogTraitsHelper<CmpStatCsvTraits>
  {
    typedef DeferredCmpStatDbSaver DbSaverType;
    typedef DeferredCmpStatPgCsvSaver PgCsvSaverType;
  };

  typedef Aux_::ConversionToOtherType<SiteStatTraits, SiteUserStatTraits>
    SiteStatExtTraits;

  struct SiteUserStatExtTraits:
    // Convert SiteStat -> SiteUserStat
    public Aux_::OtherTypeConversion<SiteStatExtTraits, SiteUserStatCsvTraits>
  {
    static
    void
    search_files(
      const std::string& in_dir,
      const FileReceiver_var& file_receiver)
    {
      static std::string conv_dir =
        in_dir.substr(0, in_dir.size() - strlen(log_base_name())) +
        SiteStatTraits::log_base_name();

      file_receiver->fetch_files(
        in_dir.c_str(),
        SiteUserStatTraits::log_base_name());

      file_receiver->fetch_files(
        conv_dir.c_str(),
        SiteStatTraits::log_base_name());
    }

    template <typename LogVersionManager>
    static void
    add_conversion_support(LogVersionManager* log_version_manager)
      /*throw(eh::Exception)*/;

    template <typename SiteStatCollector>
    static SiteStatToSiteUserStatLoader*
    make_conversion_loader(const CollectorBundlePtrType& bundle)
      /*throw(eh::Exception)*/
    {
      return new SiteStatToSiteUserStatLoader(bundle);
    }
  };

  typedef Aux_::BaseLogTraitsHelper<TagAuctionStatCsvTraits>
    TagAuctionStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<PageLoadsDailyStatCsvTraits>
    PageLoadsDailyStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<ChannelCountStatCsvTraits>
    ChannelCountStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<WebStatCsvTraits>
    WebStatExtTraits;

  typedef Aux_::BaseLogTraitsHelper<CampaignReferrerStatCsvTraits>
    CampaignReferrerStatExtTraits;
} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_LOG_TYPE_EXT_TRAITS_HPP */

