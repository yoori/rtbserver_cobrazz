
#include <memory>
#include <list>
#include <string>
#include <assert.h>

#include <eh/Exception.hpp>
#include <String/StringManip.hpp>
#include <Generics/Proc.hpp>

#include <Commons/GrpcAlgs.hpp>
#include <Commons/WireAlgs.hpp>
#include <Commons/Algs.hpp>

#include <CampaignSvcs/CampaignCommons/CorbaCampaignTypes.hpp>
#include "NonLinkedExpressionChannelCorbaAdapter.hpp"
#include "CampaignServerLogger.hpp"
#include "CampaignServerImpl.hpp"

namespace
{
  const Generics::Time CHECK_TASK_TIMEOUT_PERIOD(1);
}

namespace AdServer::CampaignSvcs::Aspect
{
  const char CAMPAIGN_SERVER[] = "CampaignServer";
}

namespace AdServer::CampaignSvcs
{
  namespace
  {
    void pack_brief_simple_channel(
      AdServer::CampaignSvcs::BriefSimpleChannelKey& ch_inf,
      const SimpleChannelDef& in)
      noexcept
    {
      ch_inf.channel_id = in.channel_id;
      ch_inf.behav_param_list_id = in.behav_param_list_id;
      ch_inf.str_behav_param_list_id = in.str_behav_param_list_id;
      ch_inf.threshold = in.threshold;

      ch_inf.categories.assign(in.categories.begin(), in.categories.end());
    }

    void pack_simple_channel(
      AdServer::CampaignSvcs::SimpleChannelKey& ch_inf,
      const SimpleChannelDef& in)
      noexcept
    {
      ch_inf.channel_id = in.channel_id;
      ch_inf.status = in.status;
      ch_inf.country_code = in.country;
      ch_inf.behav_param_list_id = in.behav_param_list_id;
      ch_inf.str_behav_param_list_id = in.str_behav_param_list_id;
      ch_inf.threshold = in.threshold;
      if (in.match_params.in())
      {
        ch_inf.page_triggers.assign(
          in.match_params->page_triggers.begin(), in.match_params->page_triggers.end());
        ch_inf.search_triggers.assign(
          in.match_params->search_triggers.begin(), in.match_params->search_triggers.end());
        ch_inf.url_triggers.assign(
          in.match_params->url_triggers.begin(), in.match_params->url_triggers.end());
        ch_inf.url_keyword_triggers.assign(
          in.match_params->url_keyword_triggers.begin(),
          in.match_params->url_keyword_triggers.end());
      }
      ch_inf.categories.assign(in.categories.begin(), in.categories.end());
      ch_inf.timestamp = GrpcAlgs::pack_time(in.timestamp);
    }

    unsigned long hash_adapter(std::uint32_t val)
    {
      return val;
    }

    unsigned long hash_adapter(const std::string& val)
    {
      return Generics::CRC::quick(0, val.data(), val.size());
    }

    template<typename SourceType, typename TargetSequenceType>
    void
    fill_deleted_sequence(
      TargetSequenceType& seq,
      const TimestampValue& request_timestamp,
      const SourceType& table,
      unsigned long portion,
      unsigned long portions_number)
      noexcept
    {
      std::uint32_t len = 0;

      for (typename SourceType::const_iterator it = table.begin(); it != table.end(); ++it)
      {
        if (hash_adapter(it->first) % portions_number == portion &&
           it->second.timestamp > request_timestamp)
        {
          ++len;
        }
      }

      seq.resize(len);

      std::uint32_t i = 0;

      for (typename SourceType::const_iterator it = table.begin(); it != table.end(); ++it)
      {
        if (hash_adapter(it->first) % portions_number == portion &&
           it->second.timestamp > request_timestamp)
        {
          seq[i].id = it->first;
          seq[i].timestamp = GrpcAlgs::pack_time(it->second.timestamp);
          ++i;
        }
      }
    }

    template<typename SourceType, typename TargetSequenceType>
    void
    fill_deleted_sequence(
      TargetSequenceType& seq,
      const TimestampValue& request_timestamp,
      const SourceType& table,
      const CampaignGetConfigSettings& settings)
      noexcept
    {
      if (!settings.no_deleted)
      {
        fill_deleted_sequence(
          seq,
          request_timestamp,
          table,
          settings.portion,
          settings.portions_number);
      }
    }

    template<
      typename ObjectMapType,
      typename DeletedIdInfoType = AdServer::CampaignSvcs::DeletedIdInfo>
    class BaseFillObjectAdapter
    {
    public:
      BaseFillObjectAdapter(
        const Generics::Time& request_timestamp,
        unsigned long portion_i,
        unsigned long portions_number)
        : request_timestamp_(request_timestamp),
          portion_i_(portion_i),
          portions_number_(portions_number)
      {}

      bool to_fill(const typename ObjectMapType::ActiveMap::value_type& value) const
      {
        return value.second->timestamp > request_timestamp_ &&
          value.first % portions_number_ == portion_i_;
      }

      bool to_deactivate(const typename ObjectMapType::
        ActiveMap::value_type& /*value*/) const
      {
        return false;
      }

      void deactivate(
        DeletedIdInfoType& res,
        const typename ObjectMapType::
          InactiveMap::value_type& value)
        const
      {
        res.id = value.first;
        res.timestamp = GrpcAlgs::pack_time(value.second.timestamp);
      }

      void deactivate(
        DeletedIdInfoType& res,
        const typename ObjectMapType::
          ActiveMap::value_type& value)
        const
      {
        res.id = value.first;
        res.timestamp = GrpcAlgs::pack_time(value.second->timestamp);
      }

    protected:
      Generics::Time request_timestamp_;
      unsigned long portion_i_;
      unsigned long portions_number_;
    };

    class GeoChannelFillAdapter: public BaseFillObjectAdapter<GeoChannelMap>
    {
    public:
      GeoChannelFillAdapter(
        const Generics::Time& request_timestamp,
        unsigned long portion_i,
        unsigned long portions_number,
        const StringSet* countries)
        : BaseFillObjectAdapter<GeoChannelMap>(request_timestamp, portion_i, portions_number),
          countries_(countries)
      {}

      void activate(
        AdServer::CampaignSvcs::GeoChannelInfo& info,
        const GeoChannelMap::ActiveMap::value_type& value)
        const
      {
        info.channel_id = value.first;
        info.country = value.second->country;
        info.timestamp = GrpcAlgs::pack_time(value.second->timestamp);

        info.geoip_targets.resize(value.second->geoip_targets.size());
        std::uint32_t i = 0;
        for (GeoChannelDef::GeoIPTargetArray::const_iterator gt_it =
              value.second->geoip_targets.begin();
            gt_it != value.second->geoip_targets.end(); ++gt_it, ++i)
        {
          info.geoip_targets[i].region = gt_it->region;
          info.geoip_targets[i].city = gt_it->city;
        }
      }

      bool to_deactivate(const GeoChannelMap::ActiveMap::value_type& value) const
      {
        return !countries_->empty() &&
                 (countries_->find(value.second->country) == countries_->end());
      }

    private:
      const StringSet* countries_;
    };

    class GeoCoordChannelFillAdapter:
      public BaseFillObjectAdapter<GeoCoordChannelMap>
    {
    public:
      GeoCoordChannelFillAdapter(
        const Generics::Time& request_timestamp,
        unsigned long portion_i,
        unsigned long portions_number)
        : BaseFillObjectAdapter<GeoCoordChannelMap>(request_timestamp, portion_i, portions_number)
      {}

      void activate(
        AdServer::CampaignSvcs::GeoCoordChannelInfo& info,
        const GeoCoordChannelMap::ActiveMap::value_type& value)
        const
      {
        info.channel_id = value.first;
        info.longitude = GrpcAlgs::pack_decimal(value.second->longitude);
        info.latitude = GrpcAlgs::pack_decimal(value.second->latitude);
        info.radius = GrpcAlgs::pack_decimal(value.second->radius);
        info.timestamp = GrpcAlgs::pack_time(value.second->timestamp);
      }
    };

    class OptionFillAdapter: public BaseFillObjectAdapter<
      CreativeOptionMap, DeletedIdInfo>
    {
    public:
      OptionFillAdapter(
        const Generics::Time& request_timestamp,
        unsigned long portion_i,
        unsigned long portions_number)
        : BaseFillObjectAdapter<CreativeOptionMap, DeletedIdInfo>(
            request_timestamp, portion_i, portions_number)
      {}

      bool to_fill(const CreativeOptionMap::ActiveMap::value_type& value) const
      {
        return value.second->timestamp > request_timestamp_ &&
          value.first % portions_number_ == portion_i_;
      }

      void activate(
        AdServer::CampaignSvcs::CreativeOptionInfo& info,
        const CreativeOptionMap::ActiveMap::value_type& value)
        const
      {
        info.option_id = value.first;
        info.token = value.second->token;
        info.type = value.second->type;
        info.timestamp = GrpcAlgs::pack_time(value.second->timestamp);

        info.token_relations.assign(
          value.second->token_relations.begin(), value.second->token_relations.end());
      }
    };

    class WebBrowserFillAdapter: public BaseFillObjectAdapter<
      WebBrowserMap, DeletedStringIdInfo>
    {
    public:
      WebBrowserFillAdapter(
        const Generics::Time& request_timestamp,
        unsigned long portion_i,
        unsigned long portions_number)
        : BaseFillObjectAdapter<WebBrowserMap, DeletedStringIdInfo>(
            request_timestamp, portion_i, portions_number)
      {}

      bool to_fill(const WebBrowserMap::ActiveMap::value_type& value) const
      {
        return value.second->timestamp > request_timestamp_ &&
          Generics::CRC::quick(0, value.first.data(), value.first.size()) %
            portions_number_ == portion_i_;
      }

      void activate(
        AdServer::CampaignSvcs::WebBrowserInfo& info,
        const WebBrowserMap::ActiveMap::value_type& value)
        const
      {
        info.name = value.first;
        info.detectors.resize(value.second->detectors.size());
        std::uint32_t i = 0;
        for (WebBrowser::DetectorList::const_iterator it = value.second->detectors.begin();
            it != value.second->detectors.end(); ++it, ++i)
        {
          info.detectors[i].marker = it->marker;
          info.detectors[i].regexp = it->regexp;
          info.detectors[i].regexp_required = it->regexp_required;
          info.detectors[i].priority = it->priority;
        }
        info.timestamp = GrpcAlgs::pack_time(value.second->timestamp);
      }
    };

    class PlatformFillAdapter: public BaseFillObjectAdapter<
      PlatformMap, DeletedIdInfo>
    {
    public:
      PlatformFillAdapter(
        const Generics::Time& request_timestamp,
        unsigned long portion_i,
        unsigned long portions_number)
        : BaseFillObjectAdapter<PlatformMap, DeletedIdInfo>(
            request_timestamp, portion_i, portions_number)
      {}

      bool to_fill(const PlatformMap::ActiveMap::value_type& value) const
      {
        return value.second->timestamp > request_timestamp_ &&
          value.first % portions_number_ == portion_i_;
      }

      void activate(
        AdServer::CampaignSvcs::PlatformInfo& info,
        const PlatformMap::ActiveMap::value_type& value)
        const
      {
        info.platform_id = value.first;
        info.name = value.second->name;
        info.type = value.second->type;
        info.detectors.resize(value.second->detectors.size());
        std::uint32_t i = 0;
        for (Platform::DetectorList::const_iterator it = value.second->detectors.begin();
            it != value.second->detectors.end(); ++it, ++i)
        {
          info.detectors[i].use_name = it->use_name;
          info.detectors[i].marker = it->marker;
          info.detectors[i].match_regexp = it->match_regexp;
          info.detectors[i].output_regexp = it->output_regexp;
          info.detectors[i].priority = it->priority;
        }
        info.timestamp = GrpcAlgs::pack_time(value.second->timestamp);
      }
    };

    class WebOperationFillAdapter: public BaseFillObjectAdapter<
      WebOperationMap,
      DeletedIdInfo>
    {
    public:
      WebOperationFillAdapter(
        const Generics::Time& request_timestamp,
        unsigned long portion_i,
        unsigned long portions_number)
        : BaseFillObjectAdapter<WebOperationMap, DeletedIdInfo>(
            request_timestamp, portion_i, portions_number)
      {}

      bool to_fill(const WebOperationMap::ActiveMap::value_type& value) const
      {
        return value.second->timestamp > request_timestamp_ &&
          value.first % portions_number_ == portion_i_;
      }

      void activate(
        AdServer::CampaignSvcs::WebOperationInfo& info,
        const WebOperationMap::ActiveMap::value_type& value)
        const
      {
        info.id = value.first;
        info.app = value.second->app;
        info.source = value.second->source;
        info.operation = value.second->operation;
        info.flags = value.second->flags;
        info.timestamp = GrpcAlgs::pack_time(value.second->timestamp);
      }
    };
  }

  CampaignServerBaseImpl::CampaignServerBaseImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    ProcStatImpl* proc_stat_impl,
    unsigned int config_update_period,
    unsigned int ecpm_update_period,
    const Generics::Time& bill_stat_update_period,
    const LogFlushTraits& colo_update_flush_traits,
    unsigned long colo,
    const char* version)
    /*throw(CampaignServerBaseImpl::InvalidArgument,
      CampaignServerBaseImpl::Exception,
      eh::Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      proc_stat_impl_(ReferenceCounting::add_ref(proc_stat_impl)),
      task_runner_(new Generics::TaskRunner(callback_, 3, 10 * 1024 * 1024)),
      scheduler_(new Generics::Planner(callback_)),
      config_update_period_(config_update_period),
      ecpm_update_period_(ecpm_update_period),
      bill_stat_update_period_(bill_stat_update_period),
      colo_id_(colo),
      version_(version)
  {
    static const char* FUN = "CampaignServerBaseImpl::CampaignServerBaseImpl()";

    if (callback == 0)
    {
      throw InvalidArgument(std::string(FUN) + ": callback == 0");
    }

    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": CompositeActiveObject::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      campaign_server_logger_ = new CampaignServerLogger(colo_update_flush_traits);
      flush_period_ = Generics::Time(colo_update_flush_traits.period);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init logger: eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      task_runner_->enqueue_task(
        Generics::Task_var(new UpdateConfigTaskMessage(this, task_runner_)));

      if (bill_stat_update_period_ != Generics::Time::ZERO)
      {
        task_runner_->enqueue_task(Generics::Task_var(new UpdateBillStatTask(this, task_runner_)));
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't enqueue first config update task: "
        "eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      TaskMessageBase_var msg = new FlushLogsTask(this, task_runner_);
      task_runner_->enqueue_task(msg);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't enqueue first flush logs task: "
        "eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  CampaignServerBaseImpl::~CampaignServerBaseImpl() noexcept
  {}

  void CampaignServerBaseImpl::change_db_state(bool /*new_state*/)
    /*throw(Exception)*/
  {
    throw Exception("Not supported");
  }

  bool CampaignServerBaseImpl::get_db_state()
    /*throw(Commons::DbStateChanger::NotSupported)*/
  {
    throw Commons::DbStateChanger::NotSupported("");
  }


  CampaignConfig_var
  CampaignServerBaseImpl::campaign_config() /*throw(eh::Exception)*/
  {
    return campaign_config_.get();
  }

  void
  CampaignServerBaseImpl::update_config() noexcept
  {
    static const char* FUN = "CampaignServerBaseImpl::update_config";

    CampaignConfig_var new_config;

    bool need_logging = false;

    try
    {
      new_config = get_campaign_config_source()->update(&need_logging);

      if (new_config.in())
      {
        proc_stat_impl_->fill_values(new_config);
        campaign_config_ = new_config;
      }
    }
    catch(const eh::Exception& e)
    {
      logger_->sstream(Logging::Logger::CRITICAL, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-149") <<
        FUN << ": caught eh::Exception on update config: " << e.what();
    }

    try
    {
      /* log campaign update */
      if (need_logging && new_config.in() && colo_id_ != 0)
      {
        CampaignServerLogger::ConfigUpdateInfo config_update_info;
        config_update_info.colo_id = colo_id_;
        config_update_info.time = Generics::Time::get_time_of_day();
        config_update_info.version = version_;

        campaign_server_logger_->process_config_update(config_update_info);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR, Aspect::CAMPAIGN_SERVER) << FUN <<
        ": Can't log config updating stat: eh::Exception caught: " << ex.what();
    }

    try
    {
      TaskMessageBase_var msg = new UpdateConfigTaskMessage(this, task_runner_);

      scheduler_->schedule(msg, Generics::Time::get_time_of_day() + config_update_period_);
    }
    catch(const eh::Exception& e)
    {
      logger_->sstream(Logging::Logger::EMERGENCY, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-148") <<
        FUN << ": can't schedule a new update task: " << e.what();
    }
  }

  void
  CampaignServerBaseImpl::update_bill_stat_() noexcept
  {
    static const char* FUN = "CampaignServerBaseImpl::update_bill_stat_()";

    Generics::Time update_period = bill_stat_update_period_;

    try
    {
      BillStatSource::Stat_var new_bill_stat =
        get_bill_stat_source()->update(nullptr, Generics::Time::get_time_of_day());

      if (new_bill_stat.in())
      {
        bill_stat_ = new_bill_stat;
      }
    }
    catch(const eh::Exception& e)
    {
      logger_->sstream(Logging::Logger::CRITICAL, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-149") <<
        FUN << ": caught eh::Exception on update config: " << e.what();

      update_period = Generics::Time(60);
    }

    if (bill_stat_update_period_ != Generics::Time::ZERO)
    {
      try
      {
        Generics::Goal_var msg = new UpdateBillStatTask(this, task_runner_);

        scheduler_->schedule(msg, Generics::Time::get_time_of_day() + update_period);
      }
      catch(const eh::Exception& e)
      {
        logger_->sstream(Logging::Logger::EMERGENCY, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-148") <<
          FUN << ": can't schedule a new update task: " << e.what();
      }
    }
  }

  void CampaignServerBaseImpl::flush_logs_() noexcept
  {
    static const char* FUN = "CampaignServerBaseImpl::flush_logs_()";

    try
    {
      campaign_server_logger_->flush_if_required();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-151") << FUN <<
        ": Can't flush logs: Caught eh::Exception: " << ex.what();
    }

    Generics::Time tm(Generics::Time::get_time_of_day() + flush_period_);

    try
    {
      Generics::Goal_var msg(new FlushLogsTask(this, task_runner_));
      scheduler_->schedule(msg, tm);
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-152") << FUN <<
        ": Can't schedule next flush task: Caught eh::Exception: " << ex.what();
    }

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->sstream(Logging::Logger::TRACE, Aspect::CAMPAIGN_SERVER) << FUN <<
        ": flush CampaignServer logger for " << tm.get_gm_time();
    }
  }

  void
  CampaignServerBaseImpl::report_error(
    Generics::ActiveObject* /*object*/,
    Generics::ActiveObjectCallback::Severity severity,
    const char* description,
    const char* error_code) noexcept
  {
    try
    {
      Stream::Error ostr;
      ostr << "AdServer::CampaignSvcs::CampaignServerBaseImpl::report_error: "
        "forwarding error : " << description;

      callback_->report_error(severity, ostr.str(), error_code);
    }
    catch(...)
    {
    }
  }

  bool
  CampaignServerBaseImpl::need_config(const AdServer::CampaignSvcs::TimestampInfo& req_timestamp)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::need_config()";

    try
    {
      TimestampValue request_timestamp(GrpcAlgs::unpack_time(req_timestamp));
      CampaignConfig_var config = campaign_config();

      if (config.in() != 0 && config->master_stamp != request_timestamp)
      {
        return true;
      }

      return false;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();

      logger_->log(ostr.str(), Logging::Logger::ERROR, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-163");
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0; // never reach
  }

  template<
    typename ObjectActiveSeqType,
    typename ObjectDeletedSeqType,
    typename ObjectContainerType,
    typename ObjectAdapterType>
  void
  CampaignServerBaseImpl::fill_object_update_sequences_(
    ObjectActiveSeqType& activate_object_seq,
    ObjectDeletedSeqType* delete_object_seq,
    const ObjectContainerType& object_container,
    const ObjectAdapterType& object_adapter,
    const char* FILL_OBJECT_TYPE)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "CampaignServerImpl::fill_object_update_sequences_<>()";

    try
    {
      std::uint32_t delete_count = object_container.inactive().size();
      std::uint32_t active_count = 0;
      for (typename ObjectContainerType::ActiveMap::const_iterator it =
            object_container.active().begin();
          it != object_container.active().end(); ++it)
      {
        if (object_adapter.to_fill(*it))
        {
          if (object_adapter.to_deactivate(*it))
          {
            ++delete_count;
          }
          else
          {
            ++active_count;
          }
        }
      }

      activate_object_seq.resize(active_count);
      if (delete_object_seq)
      {
        delete_object_seq->resize(delete_count);
      }

      std::uint32_t delete_i = 0;
      std::uint32_t active_i = 0;

      if (delete_object_seq)
      {
        for (typename ObjectContainerType::InactiveMap::const_iterator it =
              object_container.inactive().begin();
            it != object_container.inactive().end(); ++it, ++delete_i)
        {
          object_adapter.deactivate((*delete_object_seq)[delete_i], *it);
        }
      }

      for (typename ObjectContainerType::ActiveMap::const_iterator it =
            object_container.active().begin();
          it != object_container.active().end(); ++it)
      {
        if (object_adapter.to_fill(*it))
        {
          if (!object_adapter.to_deactivate(*it))
          {
            object_adapter.activate(activate_object_seq[active_i++], *it);
          }
          else if (delete_object_seq)
          {
            object_adapter.deactivate((*delete_object_seq)[delete_i++], *it);
          }
        }
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught at filling '" << FILL_OBJECT_TYPE << "': " << e.what();
      throw Exception(ostr.str());
    }
  }

  void
  CampaignServerBaseImpl::fill_creative_categories_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::fill_creative_categories_()";

    try
    {
      const CreativeCategoryMap& creative_categories = campaign_config->creative_categories;

      update_info.creative_categories.resize(creative_categories.active().size());

      std::uint32_t i = 0;
      for (CreativeCategoryMap::ActiveMap::const_iterator cat_it =
            creative_categories.active().begin();
          cat_it != creative_categories.active().end();
          ++cat_it)
      {
        if (cat_it->first % settings.portions_number == settings.portion &&
           cat_it->second->timestamp > request_timestamp)
        {
          AdServer::CampaignSvcs::CreativeCategoryInfo& cat_info =
            update_info.creative_categories[i++];
          cat_info.creative_category_id = cat_it->first;
          cat_info.cct_id = cat_it->second->cct_id;
          cat_info.name = cat_it->second->name;
          cat_info.external_categories.resize(cat_it->second->external_categories.size());
          std::uint32_t ec_i = 0;
          for (CreativeCategoryDef::ExternalCategoryMap::const_iterator ec_it =
                cat_it->second->external_categories.begin();
              ec_it != cat_it->second->external_categories.end();
              ++ec_it, ++ec_i)
          {
            cat_info.external_categories[ec_i].ad_request_type = ec_it->first;
            cat_info.external_categories[ec_i].names.assign(
              ec_it->second.begin(), ec_it->second.end());
          }

          cat_info.timestamp = GrpcAlgs::pack_time(cat_it->second->timestamp);
        }
      }

      update_info.creative_categories.resize(i);

      fill_deleted_sequence(
        update_info.deleted_creative_categories,
        request_timestamp,
        campaign_config->creative_categories.inactive(),
        settings);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  CampaignServerBaseImpl::fill_adv_actions_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::fill_adv_actions_()";

    try
    {
      unsigned long len = 0;
      const AdvActionMap::ActiveMap& adv_actions = campaign_config->adv_actions.active();

      for (AdvActionMap::ActiveMap::const_iterator it = adv_actions.begin();
          it != adv_actions.end(); ++it)
      {
        if (request_timestamp < it->second->timestamp &&
           it->first % settings.portions_number == settings.portion)
        {
          ++len;
        }
      }

      update_info.adv_actions.resize(len);
      std::uint32_t i = 0;

      for (AdvActionMap::ActiveMap::const_iterator it = adv_actions.begin();
          it != adv_actions.end(); ++it)
      {
        if (request_timestamp < it->second->timestamp &&
           it->first % settings.portions_number == settings.portion)
        {
          AdServer::CampaignSvcs::AdvActionInfo& result_adv_action_info =
            update_info.adv_actions[i];

          result_adv_action_info.action_id = it->first;
          result_adv_action_info.timestamp = GrpcAlgs::pack_time(it->second->timestamp);

          result_adv_action_info.ccg_ids.assign(
            it->second->ccg_ids.begin(), it->second->ccg_ids.end());

          WireAlgs::pack_decimal_into_seq(result_adv_action_info.ccg_ids, it->second->cur_value);

          ++i;
        }
      }

      fill_deleted_sequence(
        update_info.deleted_adv_actions,
        request_timestamp,
        campaign_config->adv_actions.inactive(),
        settings);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  CampaignServerBaseImpl::fill_category_channels_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::fill_category_channels_()";

    try
    {
      const CategoryChannelMap::ActiveMap& category_channels =
        campaign_config->category_channels.active();

      unsigned long len = count_updated_instances_(
        category_channels,
        request_timestamp,
        settings.portion,
        settings.portions_number,
        AdServer::Commons::PointerTimestampOp<CategoryChannelDef_var>());

      update_info.category_channels.resize(len);
      std::uint32_t i = 0;

      for (CategoryChannelMap::ActiveMap::const_iterator it = category_channels.begin();
          it != category_channels.end(); ++it)
      {
        if (request_timestamp < it->second->timestamp &&
           it->first % settings.portions_number == settings.portion)
        {
          assert(i < len);

          AdServer::CampaignSvcs::CategoryChannelInfo& result_channel_category_info =
            update_info.category_channels[i];

          result_channel_category_info.channel_id = it->first;
          result_channel_category_info.name = it->second->name;
          result_channel_category_info.newsgate_name = it->second->newsgate_name;
          result_channel_category_info.timestamp = GrpcAlgs::pack_time(it->second->timestamp);
          result_channel_category_info.parent_channel_id = it->second->parent_channel_id;
          result_channel_category_info.flags = it->second->flags;
          result_channel_category_info.localizations.resize(it->second->localizations.size());
          std::uint32_t li = 0;
          for (CategoryChannelDef::LocalizationMap::const_iterator lit =
                it->second->localizations.begin();
              lit != it->second->localizations.end(); ++lit, ++li)
          {
            result_channel_category_info.localizations[li].language = lit->first;
            result_channel_category_info.localizations[li].name = lit->second;
          }

          ++i;
        }
      }

      fill_deleted_sequence(
        update_info.deleted_category_channels,
        request_timestamp,
        campaign_config->category_channels.inactive(),
        settings);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignServerBaseImpl::fill_campaigns_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const StringSet& countries,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    fill_deleted_sequence(
      update_info.deleted_campaigns,
      request_timestamp,
      campaign_config->campaigns.inactive(),
      settings);

    /* recalculation result_len for GetConfigSettings */
    std::uint32_t result_len = 0;
    std::uint32_t add_deleted_count = 0;

    for (CampaignMap::ActiveMap::const_iterator it = campaign_config->campaigns.active().begin();
         it != campaign_config->campaigns.active().end(); ++it)
    {
      if (request_timestamp < it->second->timestamp &&
         it->first % settings.portions_number == settings.portion)
      {
        if (filter_campaign_(
          campaign_config, *(it->second), settings.campaign_statuses.c_str(), countries))
        {
          ++result_len;
        }
        else
        {
          ++add_deleted_count;
        }
      }
    }

    /* fill campaigns */
    update_info.campaigns.resize(result_len);
    std::uint32_t del_i = update_info.deleted_campaigns.size();
    update_info.deleted_campaigns.resize(del_i + add_deleted_count);

    std::uint32_t i = 0;

    for (CampaignMap::ActiveMap::const_iterator it = campaign_config->campaigns.active().begin();
         it != campaign_config->campaigns.active().end(); ++it)
    {
      if (!(request_timestamp < it->second->timestamp &&
           it->first % settings.portions_number == settings.portion))
      {
        continue;
      }

      if (!filter_campaign_(
        campaign_config, *(it->second), settings.campaign_statuses.c_str(), countries))
      {
        /* push campaign into deleted campaigns */
        update_info.deleted_campaigns[del_i].id = it->first;
        update_info.deleted_campaigns[del_i].timestamp =
          GrpcAlgs::pack_time(it->second->timestamp);
        ++del_i;

        continue;
      }

      assert(i < result_len);

      CampaignInfo& campaign = update_info.campaigns[i];
      ++i;

      const CampaignDef& campaign_def = *(it->second);

      campaign.campaign_id = it->first;
      campaign.campaign_group_id = campaign_def.campaign_group_id;
      campaign.timestamp = GrpcAlgs::pack_time(campaign_def.timestamp);
      campaign.ccg_rate_id = campaign_def.ccg_rate_id;
      campaign.ccg_rate_type = campaign_def.ccg_rate_type;
      campaign.fc_id = campaign_def.fc_id;
      campaign.group_fc_id = campaign_def.group_fc_id;
      campaign.flags = campaign_def.flags;
      campaign.marketplace = campaign_def.marketplace;
      campaign.account_id = campaign_def.account_id;
      campaign.advertiser_id = campaign_def.advertiser_id;
      pack_non_linked_expression(campaign.stat_expression, campaign_def.stat_expression);
      pack_non_linked_expression(campaign.expression, campaign_def.expression);

      campaign.imp_revenue = GrpcAlgs::pack_decimal(campaign_def.imp_revenue);
      campaign.click_revenue = GrpcAlgs::pack_decimal(campaign_def.click_revenue);
      campaign.action_revenue = GrpcAlgs::pack_decimal(campaign_def.action_revenue);
      campaign.commision = GrpcAlgs::pack_decimal(campaign_def.commision);
      campaign.ccg_type = campaign_def.ccg_type;
      campaign.target_type = campaign_def.target_type;
      campaign.start_user_group_id = campaign_def.start_user_group_id;
      campaign.end_user_group_id = campaign_def.end_user_group_id;
      campaign.ctr_reset_id = campaign_def.ctr_reset_id;
      campaign.mode = campaign_def.mode;
      campaign.seq_set_rotate_imps = campaign_def.seq_set_rotate_imps;
      campaign.min_uid_age = GrpcAlgs::pack_time(campaign_def.min_uid_age);

      pack_delivery_limits(
        campaign.campaign_delivery_limits,
        campaign_def.campaign_delivery_limits);
      pack_delivery_limits(campaign.ccg_delivery_limits, campaign_def.ccg_delivery_limits);

      campaign.max_pub_share = GrpcAlgs::pack_decimal(campaign_def.max_pub_share);
      campaign.bid_strategy = campaign_def.bid_strategy;
      campaign.min_ctr_goal = GrpcAlgs::pack_decimal(campaign_def.min_ctr_goal);
      campaign.min_vtr_goal = GrpcAlgs::pack_decimal(campaign_def.min_vtr_goal);

      campaign.country = campaign_def.country;
      campaign.status = campaign_def.status;
      campaign.eval_status = campaign_def.eval_status;
      campaign.delivery_coef = campaign_def.delivery_coef;

      campaign.timestamp = GrpcAlgs::pack_time(campaign_def.timestamp);

      campaign.weekly_run_intervals.resize(campaign_def.weekly_run_intervals.size());

      fill_interval_sequence(campaign.weekly_run_intervals, campaign_def.weekly_run_intervals);

      campaign.sites.assign(campaign_def.sites.begin(), campaign_def.sites.end());

      unsigned int creatives_len = campaign_def.creatives.size();
      campaign.creatives.resize(creatives_len);
      CreativeList::const_iterator jt = campaign_def.creatives.begin();
      for (unsigned int j = 0; j < creatives_len; j++, jt++)
      {
        const CreativeDef& creative = *(*jt);
        CreativeInfo& creative_info = campaign.creatives[j];
        creative_info.ccid = creative.ccid;
        creative_info.creative_id = creative.creative_id;
        creative_info.fc_id = creative.fc_id;
        creative_info.weight = creative.weight;
        creative_info.creative_format = creative.format;
        creative_info.version_id = creative.version_id;
        creative_info.click_url.option_id = creative.sys_options.click_url_option_id;
        creative_info.click_url.value = creative.sys_options.click_url;
        creative_info.html_url.option_id = creative.sys_options.html_url_option_id;
        creative_info.html_url.value = creative.sys_options.html_url;
        creative_info.order_set_id = creative.order_set_id;
        creative_info.status = creative.status;
        creative_info.initial_contract_id = creative.initial_contract_id;

        creative_info.sizes.resize(creative.sizes.size());
        std::uint32_t size_i = 0;
        for (CreativeDef::SizeMap::const_iterator size_it = creative.sizes.begin();
            size_it != creative.sizes.end(); ++size_it, ++size_i)
        {
          CreativeSizeInfo& size_info = creative_info.sizes[size_i];
          size_info.size_id = size_it->first;
          size_info.up_expand_space = size_it->second.up_expand_space;
          size_info.right_expand_space = size_it->second.right_expand_space;
          size_info.down_expand_space = size_it->second.down_expand_space;
          size_info.left_expand_space = size_it->second.left_expand_space;
          pack_option_value_map(creative_info.sizes[size_i].tokens, size_it->second.tokens);
        }

        creative_info.categories.assign(creative.categories.begin(), creative.categories.end());

        pack_option_value_map(creative_info.tokens, creative.tokens);
      }

      campaign.colocations.assign(campaign_def.colocations.begin(), campaign_def.colocations.end());

      campaign.exclude_pub_accounts.assign(
        campaign_def.exclude_pub_accounts.begin(), campaign_def.exclude_pub_accounts.end());

      std::uint32_t res_i = 0;
      campaign.exclude_tags.resize(campaign_def.exclude_tags.size());
      for (TagDeliveryMap::const_iterator dtag_it = campaign_def.exclude_tags.begin();
          dtag_it != campaign_def.exclude_tags.end(); ++dtag_it, ++res_i)
      {
        campaign.exclude_tags[res_i].tag_id = dtag_it->first;
        campaign.exclude_tags[res_i].delivery_value = dtag_it->second;
      }
    } /* campaigns cycle */
  }

  void CampaignServerBaseImpl::fill_ecpms_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const char* /*campaign_statuses*/,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    /* fill ecpms */
    update_info.ecpms.resize(campaign_config->ecpms.active().size());
    update_info.deleted_ecpms.resize(campaign_config->ecpms.inactive().size());

    unsigned long ecpms_size = 0;

    for (EcpmMap::ActiveMap::const_iterator e_it = campaign_config->ecpms.active().begin();
         e_it != campaign_config->ecpms.active().end(); ++e_it)
    {
      if (e_it->second->timestamp > request_timestamp &&
         e_it->first % settings.portions_number == settings.portion)
      {
        update_info.ecpms[ecpms_size].ccg_id = e_it->first;
        update_info.ecpms[ecpms_size].ecpm = GrpcAlgs::pack_decimal(e_it->second->ecpm);
        update_info.ecpms[ecpms_size].ctr = GrpcAlgs::pack_decimal(e_it->second->ctr);
        update_info.ecpms[ecpms_size].timestamp = GrpcAlgs::pack_time(e_it->second->timestamp);
        ++ecpms_size;
      }
    }
    update_info.ecpms.resize(ecpms_size);

    unsigned long d_ecpms_size = 0;
    for (EcpmMap::InactiveMap::const_iterator d_it = campaign_config->ecpms.inactive().begin();
         d_it != campaign_config->ecpms.inactive().end(); ++d_it)
    {
      if (d_it->second.timestamp > request_timestamp &&
          d_it->first % settings.portions_number == settings.portion)
      {
        update_info.deleted_ecpms[d_ecpms_size].id = d_it->first;
        update_info.deleted_ecpms[d_ecpms_size].timestamp =
          GrpcAlgs::pack_time(d_it->second.timestamp);
        ++d_ecpms_size;
      }
    }
    update_info.deleted_ecpms.resize(d_ecpms_size);
  }

  void CampaignServerBaseImpl::fill_accounts_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    fill_deleted_sequence(
      update_info.deleted_accounts,
      request_timestamp,
      campaign_config->accounts.inactive(),
      settings);

    std::uint32_t del_acc_i = update_info.deleted_accounts.size();
    if (!settings.no_deleted)
    {
      update_info.deleted_accounts.resize(del_acc_i + campaign_config->accounts.active().size());
    }
    update_info.accounts.resize(campaign_config->accounts.active().size());
    std::uint32_t acc_i = 0;

    for (AccountMap::ActiveMap::const_iterator it = campaign_config->accounts.active().begin();
         it != campaign_config->accounts.active().end(); ++it)
    {
      if (request_timestamp < it->second->timestamp &&
         it->first % settings.portions_number == settings.portion)
      {
        if (settings.campaign_statuses[0] == 0 ||
           ::strchr(settings.campaign_statuses.c_str(), it->second->get_status()) != 0)
        {
          const AccountDef& acc = *(it->second);
          AccountInfo& account = update_info.accounts[acc_i];

          account.account_id = it->first;
          account.agency_account_id = acc.agency_account_id;
          account.internal_account_id = acc.internal_account_id;
          account.role_id = acc.role_id;
          account.legal_name = acc.legal_name;
          account.flags = acc.flags;
          account.at_flags = acc.at_flags;
          account.text_adserving = acc.text_adserving;
          account.country = acc.country;
          account.currency_id = acc.currency_id;
          account.commision = GrpcAlgs::pack_decimal(acc.commision);
          account.budget = GrpcAlgs::pack_decimal(acc.budget);
          account.paid_amount = GrpcAlgs::pack_decimal(acc.paid_amount);
          account.timestamp = GrpcAlgs::pack_time(acc.timestamp);
          account.time_offset = GrpcAlgs::pack_time(acc.time_offset);
          account.walled_garden_accounts.assign(
            acc.walled_garden_accounts.begin(), acc.walled_garden_accounts.end());
          account.auction_rate = static_cast<std::uint32_t>(acc.auction_rate);
          account.use_pub_pixels = acc.use_pub_pixels;
          account.pub_pixel_optin = acc.pub_pixel_optin;
          account.pub_pixel_optout = acc.pub_pixel_optout;
          account.self_service_commission = GrpcAlgs::pack_decimal(acc.self_service_commission);
          account.status = acc.status;
          account.eval_status = acc.eval_status;

          ++acc_i;
        }
        else if (!settings.no_deleted)
        {
          update_info.deleted_accounts[del_acc_i].id = it->first;
          update_info.deleted_accounts[del_acc_i].timestamp =
            GrpcAlgs::pack_time(it->second->timestamp);
          ++del_acc_i;
        }
      }
    }
    update_info.deleted_accounts.resize(del_acc_i);
    update_info.accounts.resize(acc_i);
  }

  void CampaignServerBaseImpl::fill_colocations_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    update_info.colocations.resize(campaign_config->colocations.active().size());
    std::uint32_t colo_i = 0;
    for (ColocationMap::ActiveMap::const_iterator it =
           campaign_config->colocations.active().begin();
         it != campaign_config->colocations.active().end(); ++it)
    {
      if (request_timestamp < it->second->timestamp &&
         it->first % settings.portions_number == settings.portion)
      {
        ColocationInfo& colo_info = update_info.colocations[colo_i];
        const ColocationDef& colo = *(it->second);
        colo_info.colo_id = colo.colo_id;
        colo_info.colo_name = colo.colo_name;
        colo_info.colo_rate_id = colo.colo_rate_id;
        colo_info.account_id = colo.account_id;
        colo_info.at_flags = colo.at_flags;
        colo_info.revenue_share = GrpcAlgs::pack_decimal(colo.revenue_share);
        colo_info.ad_serving = colo.ad_serving;
        colo_info.hid_profile = colo.hid_profile;
        pack_option_value_map(colo_info.tokens, colo.tokens);
        colo_info.timestamp = GrpcAlgs::pack_time(colo.timestamp);

        ++colo_i;
      }
    }
    update_info.colocations.resize(colo_i);

    fill_deleted_sequence(
      update_info.deleted_colocations,
      request_timestamp,
      campaign_config->colocations.inactive(),
      settings);
  }

  void
  CampaignServerBaseImpl::fill_contracts_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    noexcept
  {
    fill_deleted_sequence(
      update_info.deleted_contracts,
      request_timestamp,
      campaign_config->contracts.inactive(),
      settings);

    update_info.contracts.resize(campaign_config->contracts.active().size());
    std::uint32_t res_contract_i = 0;
    for (auto it = campaign_config->contracts.active().begin(); it != campaign_config->contracts.active().end(); ++it)
    {
      if (request_timestamp < it->second->timestamp &&
         it->first % settings.portions_number == settings.portion)
      {
        ContractInfo& res_contract = update_info.contracts[res_contract_i];
        const ContractDef& contract = *(it->second);
        res_contract.contract_id = contract.contract_id;

        res_contract.number = contract.number;
        res_contract.date = contract.date;
        res_contract.type = contract.type;
        res_contract.vat_included = contract.vat_included;

        res_contract.ord_contract_id = contract.ord_contract_id;
        res_contract.ord_ado_id = contract.ord_ado_id;
        res_contract.subject_type = contract.subject_type;
        res_contract.action_type = contract.action_type;
        res_contract.agent_acting_for_publisher = contract.agent_acting_for_publisher;
        res_contract.parent_contract_id = contract.parent_contract_id;

        res_contract.client_id = contract.client_id;
        res_contract.client_name = contract.client_name;
        res_contract.client_legal_form = contract.client_legal_form;

        res_contract.contractor_id = contract.contractor_id;
        res_contract.contractor_name = contract.contractor_name;
        res_contract.contractor_legal_form = contract.contractor_legal_form;
        res_contract.timestamp = GrpcAlgs::pack_time(contract.timestamp);

        ++res_contract_i;
      }
    }

    update_info.contracts.resize(res_contract_i);
  }

  void CampaignServerBaseImpl::fill_active_freq_caps_(
    FreqCapSeq& freq_cap_seq,
    const TimestampValue& request_timestamp,
    const FreqCapMap::ActiveMap& freq_caps,
    unsigned long portion,
    unsigned long portions_number)
    /*throw(Exception)*/
  {
    freq_cap_seq.resize(freq_caps.size());
    std::uint32_t freq_i = 0;
    for (FreqCapMap::ActiveMap::const_iterator fit = freq_caps.begin();
        fit != freq_caps.end(); ++fit)
    {
      if (request_timestamp < fit->second->timestamp && fit->first % portions_number == portion)
      {
        FreqCapInfo& freq_cap = freq_cap_seq[freq_i];
        const FreqCapDef& fc = *(fit->second);

        freq_cap.fc_id = fc.fc_id;
        freq_cap.lifelimit = fc.lifelimit;
        freq_cap.period = fc.period;
        freq_cap.window_limit = fc.window_limit;
        freq_cap.window_time = fc.window_time;
        freq_cap.timestamp = GrpcAlgs::pack_time(fc.timestamp);

        ++freq_i;
      }
    }
    freq_cap_seq.resize(freq_i);
  }

  void
  CampaignServerBaseImpl::fill_active_campaign_ids_(
    CampaignIdSeq& campaign_ids,
    const CampaignMap::ActiveMap& campaigns)
    /*throw(Exception)*/
  {
    std::set<unsigned long> campaign_ids_set;

    for (auto it = campaigns.begin(); it != campaigns.end(); ++it)
    {
      campaign_ids_set.insert(it->second->campaign_group_id);
    }

    campaign_ids.assign(campaign_ids_set.begin(), campaign_ids_set.end());
  }

  void CampaignServerBaseImpl::fill_freq_caps_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    fill_active_freq_caps_(
      update_info.frequency_caps,
      request_timestamp,
      campaign_config->freq_caps.active(),
      settings.portion,
      settings.portions_number);

    fill_deleted_sequence(
      update_info.deleted_freq_caps,
      request_timestamp,
      campaign_config->freq_caps.inactive(),
      settings);
  }

  void CampaignServerBaseImpl::fill_creative_templates_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    update_info.creative_templates.resize(campaign_config->creative_templates.active().size());
    std::uint32_t ct_res_len = 0;

    for (CreativeTemplateMap::ActiveMap::const_iterator ct_it =
           campaign_config->creative_templates.active().begin();
         ct_it != campaign_config->creative_templates.active().end();
         ++ct_it)
    {
      if (request_timestamp < ct_it->second->timestamp &&
         ct_it->first % settings.portions_number == settings.portion)
      {
        CreativeTemplateInfo& ct_info = update_info.creative_templates[ct_res_len];

        ct_info.id = ct_it->first;
        ct_info.timestamp = GrpcAlgs::pack_time(ct_it->second->timestamp);

        ct_info.files.resize(ct_it->second->files.size());
        std::uint32_t tf_i = 0;

        for (CreativeTemplateFileList::const_iterator tf_it = ct_it->second->files.begin();
            tf_it != ct_it->second->files.end();
            ++tf_it, ++tf_i)
        {
          CreativeTemplateFileInfo& ctf_info = ct_info.files[tf_i];

          ctf_info.creative_format = tf_it->creative_format;
          ctf_info.creative_size = tf_it->creative_size;
          ctf_info.app_format = tf_it->app_format;
          ctf_info.mime_format = tf_it->mime_format;
          ctf_info.track_impr = tf_it->track_impr;
          ctf_info.template_file = tf_it->template_file;
          ctf_info.type = tf_it->type;
        }

        pack_option_value_map(ct_info.tokens, ct_it->second->tokens);
        pack_option_value_map(ct_info.hidden_tokens, ct_it->second->hidden_tokens);

        ++ct_res_len;
      }
    }
    update_info.creative_templates.resize(ct_res_len);

    fill_deleted_sequence(
      update_info.deleted_templates,
      request_timestamp,
      campaign_config->creative_templates.inactive(),
      settings);
  }

  void CampaignServerBaseImpl::fill_sites_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    update_info.sites.resize(campaign_config->sites.active().size());
    std::uint32_t site_i = 0;

    for (SiteMap::ActiveMap::const_iterator pt = campaign_config->sites.active().begin();
        pt != campaign_config->sites.active().end(); ++pt)
    {
      if (request_timestamp < pt->second->timestamp &&
         pt->first % settings.portions_number == settings.portion)
      {
        SiteInfo& site_info = update_info.sites[site_i];
        site_info.site_id = pt->first;
        site_info.freq_cap_id = pt->second->freq_cap_id;
        site_info.noads_timeout = pt->second->noads_timeout;
        site_info.status = pt->second->status;
        site_info.flags = pt->second->flags;
        site_info.account_id = pt->second->account_id;

        site_info.timestamp = GrpcAlgs::pack_time(pt->second->timestamp);

        /* fill site accept & reject creative categories */
        site_info.approved_creative_categories.assign(
          pt->second->approved_creative_categories.begin(),
          pt->second->approved_creative_categories.end());

        site_info.rejected_creative_categories.assign(
          pt->second->rejected_creative_categories.begin(),
          pt->second->rejected_creative_categories.end());

        // fill site approved creatives
        site_info.approved_creatives.assign(
          pt->second->approved_creatives.begin(), pt->second->approved_creatives.end());

        // fill site rejected creatives
        site_info.rejected_creatives.assign(
          pt->second->rejected_creatives.begin(), pt->second->rejected_creatives.end());

        ++site_i;
      }
    }
    update_info.sites.resize(site_i);

    fill_deleted_sequence(
      update_info.deleted_sites,
      request_timestamp,
      campaign_config->sites.inactive(),
      settings);
  }

  void
  CampaignServerBaseImpl::fill_app_formats_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    const AppFormatMap::ActiveMap& active_app_formats = campaign_config->app_formats.active();
    update_info.app_formats.resize(active_app_formats.size());
    std::uint32_t app_formats_res_i = 0;
    for (AppFormatMap::ActiveMap::const_iterator app_format_it = active_app_formats.begin();
         app_format_it != active_app_formats.end(); ++app_format_it)
    {
      if (request_timestamp < app_format_it->second->timestamp &&
          Generics::CRC::quick(0, app_format_it->first.data(), app_format_it->first.size()) %
            settings.portions_number == settings.portion)
      {
        AppFormatInfo& app_format_info = update_info.app_formats[app_formats_res_i];
        app_format_info.app_format = app_format_it->first;
        app_format_info.mime_format = app_format_it->second->mime_format;
        app_format_info.timestamp = GrpcAlgs::pack_time(app_format_it->second->timestamp);
        ++app_formats_res_i;
      }
    }
    update_info.app_formats.resize(app_formats_res_i);

    fill_deleted_sequence(
      update_info.delete_app_formats,
      request_timestamp,
      campaign_config->app_formats.inactive(),
      settings);
  }

  void
  CampaignServerBaseImpl::fill_sizes_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    const SizeMap::ActiveMap& active_sizes = campaign_config->sizes.active();
    update_info.sizes.resize(active_sizes.size());
    std::uint32_t sizes_res_i = 0;
    for (SizeMap::ActiveMap::const_iterator size_it = active_sizes.begin();
         size_it != active_sizes.end(); ++size_it)
    {
      if (request_timestamp < size_it->second->timestamp &&
          size_it->first % settings.portions_number == settings.portion)
      {
        SizeInfo& size_info = update_info.sizes[sizes_res_i];
        size_info.size_id = size_it->first;
        size_info.protocol_name = size_it->second->protocol_name;
        size_info.size_type_id = size_it->second->size_type_id;
        size_info.width = size_it->second->width;
        size_info.height = size_it->second->height;
        size_info.timestamp = GrpcAlgs::pack_time(size_it->second->timestamp);
        ++sizes_res_i;
      }
    }
    update_info.sizes.resize(sizes_res_i);

    fill_deleted_sequence(
      update_info.delete_sizes,
      request_timestamp,
      campaign_config->sizes.inactive(),
      settings);
  }

  void
  CampaignServerBaseImpl::fill_countries_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    const CountryMap::ActiveMap& active_countries = campaign_config->countries.active();
    update_info.countries.resize(active_countries.size());
    std::uint32_t country_res_i = 0;
    for (CountryMap::ActiveMap::const_iterator country_it = active_countries.begin();
         country_it != active_countries.end(); ++country_it)
    {
      if (request_timestamp < country_it->second->timestamp &&
          Generics::CRC::quick(0, country_it->first.data(), country_it->first.size()) %
            settings.portions_number == settings.portion)
      {
        CountryInfo& country_info = update_info.countries[country_res_i];
        country_info.country_code = country_it->first;
        pack_option_value_map(country_info.tokens, country_it->second->tokens);
        country_info.timestamp = GrpcAlgs::pack_time(country_it->second->timestamp);
        ++country_res_i;
      }
    }
    update_info.countries.resize(country_res_i);

    fill_deleted_sequence(
      update_info.deleted_countries,
      request_timestamp,
      campaign_config->countries.inactive(),
      settings);
  }

  void CampaignServerBaseImpl::fill_tags_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    const TagMap::ActiveMap& active_tags = campaign_config->tags.active();
    update_info.tags.resize(active_tags.size());
    std::uint32_t tags_res_len = 0;
    for (TagMap::ActiveMap::const_iterator tt = active_tags.begin(); tt != active_tags.end(); ++tt)
    {
      if (request_timestamp < tt->second->timestamp &&
         tt->first % settings.portions_number == settings.portion)
      {
        TagInfo& tag_info = update_info.tags[tags_res_len];
        tag_info.tag_id = tt->first;
        tag_info.site_id = tt->second->site_id;
        tag_info.sizes.resize(tt->second->sizes.size());
        std::uint32_t size_i = 0;
        for (TagDef::SizeMap::const_iterator size_it = tt->second->sizes.begin();
            size_it != tt->second->sizes.end(); ++size_it, ++size_i)
        {
          tag_info.sizes[size_i].size_id = size_it->first;
          tag_info.sizes[size_i].max_text_creatives = size_it->second.max_text_creatives;
          pack_option_value_map(tag_info.sizes[size_i].tokens, size_it->second.tokens);
          pack_option_value_map(tag_info.sizes[size_i].hidden_tokens, size_it->second.hidden_tokens);
        }

        tag_info.imp_track_pixel = tt->second->imp_track_pixel;
        tag_info.passback = tt->second->passback;
        tag_info.passback_type = tt->second->passback_type;
        tag_info.allow_expandable = tt->second->allow_expandable;
        tag_info.flags = tt->second->flags;
        tag_info.marketplace = tt->second->marketplace;
        tag_info.adjustment = GrpcAlgs::pack_decimal(tt->second->adjustment);
        tag_info.auction_max_ecpm_share = GrpcAlgs::pack_decimal(
          tt->second->auction_max_ecpm_share);
        tag_info.auction_prop_probability_share = GrpcAlgs::pack_decimal(
          tt->second->auction_prop_probability_share);
        tag_info.auction_random_share = GrpcAlgs::pack_decimal(tt->second->auction_random_share);
        tag_info.cost_coef = GrpcAlgs::pack_decimal(tt->second->cost_coef);
        tag_info.tag_pricings_timestamp = GrpcAlgs::pack_time(tt->second->tag_pricings_timestamp);
        tag_info.timestamp = GrpcAlgs::pack_time(tt->second->timestamp);

        // fill sizes
        /* fill tag pricings */
        const TagPricings& tag_pricings = tt->second->tag_pricings;
        unsigned int tag_pricings_len = tag_pricings.size();
        tag_info.tag_pricings.resize(tag_pricings_len);

        std::uint32_t tp_i = 0;
        for (TagPricings::const_iterator tp_it = tag_pricings.begin();
             tp_it != tag_pricings.end(); ++tp_it, ++tp_i)
        {
          TagPricingInfo& tag_pricing = tag_info.tag_pricings[tp_i];
          tag_pricing.country_code = tp_it->country_code;
          tag_pricing.ccg_type = static_cast<char>(tp_it->ccg_type);
          tag_pricing.ccg_rate_type = static_cast<char>(tp_it->ccg_rate_type);

          tag_pricing.site_rate_id = tp_it->site_rate_id;
          tag_pricing.revenue_share = GrpcAlgs::pack_decimal(tp_it->revenue_share);
          tag_pricing.imp_revenue = GrpcAlgs::pack_decimal(tp_it->imp_revenue);
        }

        tag_info.accepted_categories.assign(
          tt->second->accepted_categories.begin(), tt->second->accepted_categories.end());

        tag_info.rejected_categories.assign(
          tt->second->rejected_categories.begin(), tt->second->rejected_categories.end());

        pack_option_value_map(tag_info.tokens, tt->second->tokens);
        pack_option_value_map(tag_info.hidden_tokens, tt->second->hidden_tokens);
        pack_option_value_map(tag_info.passback_tokens, tt->second->passback_tokens);

        tag_info.template_tokens.resize(tt->second->template_tokens.size());
        std::uint32_t template_i = 0;
        for (TemplateOptionValueMap::const_iterator t_it = tt->second->template_tokens.begin();
            t_it != tt->second->template_tokens.end();
            ++t_it, ++template_i)
        {
          tag_info.template_tokens[template_i].template_name = t_it->first;
          pack_option_value_map(tag_info.template_tokens[template_i].tokens, t_it->second);
        }

        ++tags_res_len;
      }
    }
    update_info.tags.resize(tags_res_len);

    fill_deleted_sequence(
      update_info.deleted_tags,
      request_timestamp,
      campaign_config->tags.inactive(),
      settings);
  }

  void CampaignServerBaseImpl::fill_currencies_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    update_info.currencies.resize(campaign_config->currencies.active().size());
    std::uint32_t cur_res_len = 0;

    for (CurrencyMap::ActiveMap::const_iterator currency_it =
           campaign_config->currencies.active().begin();
         currency_it != campaign_config->currencies.active().end();
         ++currency_it)
    {
      if (currency_it->second->timestamp > request_timestamp &&
         currency_it->first % settings.portions_number == settings.portion)
      {
        CurrencyInfo& currency_info = update_info.currencies[cur_res_len];
        const CurrencyDef& currency = *(currency_it->second);
        currency_info.currency_id = currency.currency_id;
        currency_info.currency_exchange_id = currency.currency_exchange_id;
        currency_info.effective_date = currency.effective_date;
        currency_info.rate = GrpcAlgs::pack_decimal(currency.rate);
        currency_info.fraction_digits = currency.fraction_digits;
        currency_info.currency_code = currency.currency_code;
        currency_info.timestamp = GrpcAlgs::pack_time(currency.timestamp);
        ++cur_res_len;
      }
    }
    update_info.currencies.resize(cur_res_len);
  }

  void CampaignServerBaseImpl::fill_campaign_keywords_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    fill_deleted_sequence(
      update_info.deleted_keywords,
      request_timestamp,
      campaign_config->campaign_keywords.inactive(),
      settings);

    update_info.campaign_keywords.resize(campaign_config->campaign_keywords.active().size());

    std::uint32_t i = 0;

    for (CampaignKeywordMap::ActiveMap::const_iterator kit =
          campaign_config->campaign_keywords.active().begin();
        kit != campaign_config->campaign_keywords.active().end(); ++kit)
    {
      if (request_timestamp < kit->second->timestamp)
      {
        AdServer::CampaignSvcs::CampaignKeywordInfo& kw_info = update_info.campaign_keywords[i];
        const CampaignKeyword& kw = *(kit->second);
        kw_info.ccg_keyword_id = kw.ccg_keyword_id;
        kw_info.original_keyword = kw.original_keyword;
        kw_info.click_url = kw.click_url;
        kw_info.timestamp = GrpcAlgs::pack_time(kw.timestamp);
        ++i;
      }
    }

    update_info.campaign_keywords.resize(i);
  }

  void CampaignServerBaseImpl::fill_brief_bp_parameters_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    unsigned long portion,
    unsigned long portions_number,
    AdServer::CampaignSvcs::BriefBehavParamInfoSeq& bp_ids,
    AdServer::CampaignSvcs::BriefKeyBehavParamInfoSeq& bp_keys)
      /*throw(eh::Exception)*/
  {
    bp_ids.resize(campaign_config->behav_param_lists.active().size());

    size_t i = 0;
    for (BehavioralParameterMap::ActiveMap::const_iterator bit =
          campaign_config->behav_param_lists.active().begin();
        bit != campaign_config->behav_param_lists.active().end(); bit++)
    {
      if (request_timestamp < bit->second->timestamp && bit->first % portions_number == portion)
      {
        AdServer::CampaignSvcs::BriefBehavParamInfo& bp_info = bp_ids[i++];
        bp_info.id = bit->first;
        bp_info.threshold = bit->second->threshold;
        bp_info.timestamp = GrpcAlgs::pack_time(bit->second->timestamp);
        bp_info.bp_seq.resize(bit->second->behave_params.size());
        std::uint32_t bp_i = 0;
        for (BehavioralParameterListDef::BehavioralParameterList::const_iterator
              bp_it = bit->second->behave_params.begin();
            bp_it != bit->second->behave_params.end(); ++bp_it, ++bp_i)
        {
          bp_info.bp_seq[bp_i].min_visits = bp_it->min_visits;
          bp_info.bp_seq[bp_i].time_from = bp_it->time_from;
          bp_info.bp_seq[bp_i].time_to = bp_it->time_to;
          bp_info.bp_seq[bp_i].weight = bp_it->weight;
          bp_info.bp_seq[bp_i].trigger_type = bp_it->trigger_type;
        }
      }
    }
    bp_ids.resize(i);

    bp_keys.resize(campaign_config->str_behav_param_lists.active().size());

    i = 0;
    for (BehavioralParameterKeyMap::ActiveMap::const_iterator bit =
          campaign_config->str_behav_param_lists.active().begin();
        bit != campaign_config->str_behav_param_lists.active().end(); ++bit)
    {
      if (request_timestamp < bit->second->timestamp &&
          Generics::CRC::quick(0, bit->first.c_str(), bit->first.size()) %
               portions_number == portion)
      {
        AdServer::CampaignSvcs::BriefKeyBehavParamInfo& bp_info = bp_keys[i++];

        bp_info.id = bit->first;
        bp_info.threshold = bit->second->threshold;
        bp_info.timestamp = GrpcAlgs::pack_time(bit->second->timestamp);
        bp_info.bp_seq.resize(bit->second->behave_params.size());
        std::uint32_t bp_i = 0;
        for (BehavioralParameterListDef::BehavioralParameterList::const_iterator
              bp_it = bit->second->behave_params.begin();
            bp_it != bit->second->behave_params.end(); ++bp_it, ++bp_i)
        {
          bp_info.bp_seq[bp_i].min_visits = bp_it->min_visits;
          bp_info.bp_seq[bp_i].time_from = bp_it->time_from;
          bp_info.bp_seq[bp_i].time_to = bp_it->time_to;
          bp_info.bp_seq[bp_i].weight = bp_it->weight;
          bp_info.bp_seq[bp_i].trigger_type = bp_it->trigger_type;
        }
      }
    }
    bp_keys.resize(i);
  }

  void CampaignServerBaseImpl::fill_bp_parameters_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    unsigned long portion,
    unsigned long portions_number,
    AdServer::CampaignSvcs::BehavParamInfoSeq& bp_ids,
    AdServer::CampaignSvcs::KeyBehavParamInfoSeq& bp_keys)
      /*throw(eh::Exception)*/
  {
    bp_ids.resize(campaign_config->behav_param_lists.active().size());

    size_t i = 0;
    for (BehavioralParameterMap::ActiveMap::const_iterator bit =
          campaign_config->behav_param_lists.active().begin();
        bit != campaign_config->behav_param_lists.active().end(); ++bit)
    {
      if (request_timestamp < bit->second->timestamp && bit->first % portions_number == portion)
      {
        AdServer::CampaignSvcs::BehavParamInfo& bp_info = bp_ids[i++];
        bp_info.id = bit->first;
        bp_info.threshold = bit->second->threshold;
        bp_info.timestamp = GrpcAlgs::pack_time(bit->second->timestamp);
        bp_info.bp_seq.resize(8);
        size_t j = 0;
        for (std::list<BehavioralParameterDef>::const_iterator bp_it =
            bit->second->behave_params.begin();
            bp_it != bit->second->behave_params.end(); ++bp_it, j++)
        {
          bp_info.bp_seq.resize(j + 1);
          bp_info.bp_seq[j].min_visits = bp_it->min_visits;
          bp_info.bp_seq[j].time_from = bp_it->time_from;
          bp_info.bp_seq[j].time_to = bp_it->time_to;
          bp_info.bp_seq[j].weight = bp_it->weight;
          bp_info.bp_seq[j].trigger_type = bp_it->trigger_type;
        }
        bp_info.bp_seq.resize(j);
      }
    }
    bp_ids.resize(i);

    bp_keys.resize(campaign_config->str_behav_param_lists.active().size());

    i = 0;
    for (BehavioralParameterKeyMap::ActiveMap::const_iterator bit =
          campaign_config->str_behav_param_lists.active().begin();
        bit != campaign_config->str_behav_param_lists.active().end(); ++bit)
    {
      if (request_timestamp < bit->second->timestamp &&
          Generics::CRC::quick(0, bit->first.c_str(), bit->first.size()) %
               portions_number == portion)
      {
        AdServer::CampaignSvcs::KeyBehavParamInfo& bp_info = bp_keys[i++];

        bp_info.id = bit->first;
        bp_info.threshold = bit->second->threshold;
        bp_info.timestamp = GrpcAlgs::pack_time(bit->second->timestamp);
        bp_info.bp_seq.resize(8);
        size_t j = 0;
        for (std::list<BehavioralParameterDef>::const_iterator bp_it =
            bit->second->behave_params.begin();
            bp_it != bit->second->behave_params.end(); ++bp_it, j++)
        {
          bp_info.bp_seq.resize(j + 1);
          bp_info.bp_seq[j].min_visits = bp_it->min_visits;
          bp_info.bp_seq[j].time_from = bp_it->time_from;
          bp_info.bp_seq[j].time_to = bp_it->time_to;
          bp_info.bp_seq[j].weight = bp_it->weight;
          bp_info.bp_seq[j].trigger_type = bp_it->trigger_type;
        }
        bp_info.bp_seq.resize(j);
      }
    }
    bp_keys.resize(i);
  }

  void CampaignServerBaseImpl::fill_behavioral_parameters_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    const char* FUN = "CampaignServerBaseImpl::fill_behavioral_parameters_";

    try
    {
      fill_deleted_sequence(
        update_info.deleted_behav_params,
        request_timestamp,
        campaign_config->behav_param_lists.inactive(),
        settings);

      fill_deleted_sequence(
        update_info.deleted_key_behav_params,
        request_timestamp,
        campaign_config->str_behav_param_lists.inactive(),
        settings);

      fill_bp_parameters_(
        request_timestamp,
        campaign_config,
        settings.portion,
        settings.portions_number,
        update_info.behav_params,
        update_info.key_behav_params);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err << FUN << ": caught eh::Exception: " << e.what();
      throw Exception(err);
    }
  }

  void CampaignServerBaseImpl::fill_fraud_conditions_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    FraudConditionSeq& fraud_condition_seq,
    DeletedIdSeq* deleted_fraud_condition_seq)
    /*throw(Exception)*/
  {
    fraud_condition_seq.resize(campaign_config->fraud_conditions.active().size());

    std::uint32_t i = 0;
    for (FraudConditionMap::ActiveMap::const_iterator it =
          campaign_config->fraud_conditions.active().begin();
        it != campaign_config->fraud_conditions.active().end();
        ++it)
    {
      if (request_timestamp < it->second->timestamp &&
           it->first % settings.portions_number == settings.portion)
      {
        AdServer::CampaignSvcs::FraudConditionInfo& fraud_cond_info = fraud_condition_seq[i++];
        fraud_cond_info.id = it->first;
        fraud_cond_info.type = it->second->type;
        fraud_cond_info.period = GrpcAlgs::pack_time(it->second->period);
        fraud_cond_info.limit = it->second->limit;
        fraud_cond_info.timestamp = GrpcAlgs::pack_time(it->second->timestamp);
      }
    }

    fraud_condition_seq.resize(i);

    if (deleted_fraud_condition_seq)
    {
      fill_deleted_sequence(
        *deleted_fraud_condition_seq,
        request_timestamp,
        campaign_config->fraud_conditions.inactive(),
        settings);
    }
  }

  void
  CampaignServerBaseImpl::fill_search_enginies_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    SearchEngineSeq& search_engines,
    AdServer::CampaignSvcs::DeletedIdSeq* del_seq)
    const
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::fill_search_enginies_()";

    try
    {
      search_engines.resize(campaign_config->search_engines.active().size());

      std::uint32_t i = 0;
      for (SearchEngineMap::ActiveMap::const_iterator it =
            campaign_config->search_engines.active().begin();
          it != campaign_config->search_engines.active().end(); it++)
      {
        if (it->second->timestamp > request_timestamp &&
           it->first % settings.portions_number == settings.portion)
        {
          SearchEngineInfo& res = search_engines[i];
          res.id = it->first;
          res.regexps.resize(it->second->regexps.size());

          std::uint32_t re_i = 0;
          for (SearchEngine::SearchEngineRegExpList::const_iterator re_it =
                it->second->regexps.begin();
              re_it != it->second->regexps.end();
              ++re_it, ++re_i)
          {
            res.regexps[re_i].host_postfix = re_it->host_postfix;
            res.regexps[re_i].regexp = re_it->regexp;
            res.regexps[re_i].encoding = re_it->encoding;
            res.regexps[re_i].post_encoding = re_it->post_encoding;
            res.regexps[re_i].decoding_depth = re_it->decoding_depth;
          }

          res.timestamp = GrpcAlgs::pack_time(it->second->timestamp);

          ++i;
        }
      }
      search_engines.resize(i);

      if (del_seq)
      {
        fill_deleted_sequence(
          *del_seq,
          request_timestamp,
          campaign_config->search_engines.inactive(),
          settings);
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught:" << e.what();
      throw Exception(ostr.str());
    }
  }

  /* client get config adapters */
  bool CampaignServerBaseImpl::filter_campaign_(
    const CampaignConfig* campaign_config,
    const CampaignDef& campaign,
    const char* campaign_statuses,
    const StringSet& countries)
    noexcept
  {
    if (campaign_statuses[0] != 0 && ::strchr(campaign_statuses, campaign.get_status()) == 0)
    {
      return false;
    }

    if (countries.empty())
    {
      return true;
    }

    AccountMap::ActiveMap::const_iterator acc_it =
      campaign_config->accounts.active().find(campaign.account_id);
    return acc_it != campaign_config->accounts.active().end() &&
      countries.find(acc_it->second->country) != countries.end();
  }

  void
  CampaignServerBaseImpl::fill_amount_distribution_(
    AdServer::CampaignSvcs::AmountDistributionInfo& amount_distribution_info,
    const BillStatSource::Stat::AmountDistribution& amount_distribution)
    noexcept
  {
    amount_distribution_info.prev_days_amount.day =
      GrpcAlgs::pack_time(amount_distribution.prev_day);
    amount_distribution_info.prev_days_amount.amount =
      GrpcAlgs::pack_decimal(amount_distribution.prev_days_amount);

    amount_distribution_info.day_amounts.resize(amount_distribution.day_amounts.size());
    std::uint32_t day_i = 0;
    for (auto day_it = amount_distribution.day_amounts.begin();
      day_it != amount_distribution.day_amounts.end();
      ++day_it, ++day_i)
    {
      amount_distribution_info.day_amounts[day_i].day = GrpcAlgs::pack_time(day_it->first);
      amount_distribution_info.day_amounts[day_i].amount = GrpcAlgs::pack_decimal(day_it->second);
    }
  }

  void
  CampaignServerBaseImpl::fill_amount_count_distribution_(
    AdServer::CampaignSvcs::AmountCountDistributionInfo& amount_count_distribution_info,
    const BillStatSource::Stat::AmountCountDistribution& amount_count_distribution)
    noexcept
  {
    amount_count_distribution_info.prev_days_amount_count.day =
      GrpcAlgs::pack_time(amount_count_distribution.prev_day);
    amount_count_distribution_info.prev_days_amount_count.amount =
      GrpcAlgs::pack_decimal(amount_count_distribution.prev_days_amount);
    amount_count_distribution_info.prev_days_amount_count.imps =
      GrpcAlgs::pack_decimal(amount_count_distribution.prev_days_imps);
    amount_count_distribution_info.prev_days_amount_count.clicks =
      GrpcAlgs::pack_decimal(amount_count_distribution.prev_days_clicks);

    amount_count_distribution_info.day_amount_counts.resize(
      amount_count_distribution.day_amount_counts.size());
    std::uint32_t day_i = 0;
    for (auto day_it = amount_count_distribution.day_amount_counts.begin();
      day_it != amount_count_distribution.day_amount_counts.end();
      ++day_it, ++day_i)
    {
      auto& res_day_amount = amount_count_distribution_info.day_amount_counts[day_i];
      res_day_amount.day = GrpcAlgs::pack_time(day_it->first);
      res_day_amount.amount = GrpcAlgs::pack_decimal(day_it->second.amount);
      res_day_amount.imps = GrpcAlgs::pack_decimal(day_it->second.imps);
      res_day_amount.clicks = GrpcAlgs::pack_decimal(day_it->second.clicks);
    }
  }

  CampaignConfigUpdateInfo*
  CampaignServerBaseImpl::get_config(const CampaignGetConfigSettings& settings)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::get_config()";

    try
    {
      TimestampValue request_timestamp = GrpcAlgs::unpack_time(settings.timestamp);

      CampaignConfig_var config = campaign_config();

      if (config.in() == 0)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": AdServer::CampaignSvcs::CampaignServer::NotReady caught: "
          "CampaignServer is not ready (there is no campaign config).";
        throw CampaignSvcs::CampaignServer::NotReady(ostr.str().str());
      }

      if (request_timestamp < config->first_load_stamp ||
         (settings.server_id != 0 && settings.server_id != config->server_id))
      {
        request_timestamp = Generics::Time::ZERO;
      }

      std::unique_ptr<CampaignConfigUpdateInfo> result(new CampaignConfigUpdateInfo());

      result->server_id = config->server_id;
      result->master_stamp = GrpcAlgs::pack_time(config->master_stamp);
      result->first_load_stamp = GrpcAlgs::pack_time(config->first_load_stamp);
      result->finish_load_stamp = GrpcAlgs::pack_time(config->finish_load_stamp);
      result->current_time = GrpcAlgs::pack_time(Generics::Time::get_time_of_day());

      if (config->master_stamp != TimestampValue(0) && config->master_stamp < request_timestamp)
      {
        logger_->sstream(Logging::Logger::WARNING, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-153") <<
          FUN << ": original master stamp = " <<
          Generics::Time(config->master_stamp).get_gm_time() <<
          ", request timestamp = " <<
          Generics::Time(request_timestamp).get_gm_time() <<
          ", portion = " << settings.portion <<
          ", portions number = " << settings.portions_number <<
          ", colo_id = " << settings.colo_id << ", version = '" << settings.version << "'";
      }

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->stream(Logging::Logger::TRACE, Aspect::CAMPAIGN_SERVER) << FUN <<
          ": process get_config with request_timestamp = " <<
          Generics::Time(request_timestamp).get_gm_time() <<
          ", client server-id = " << settings.server_id <<
          ", current server-id = " << config->server_id;
      }

      std::string campaign_statuses = settings.campaign_statuses.c_str();
      StringSet countries;

      {
        String::StringManip::SplitSpace tokenizer(String::SubString(settings.country.c_str()));
        String::SubString token;
        while (tokenizer.get_token(token))
        {
          countries.insert(token.str());
        }
      }

      result->currency_exchange_id = config->global_params.currency_exchange_id;
      result->fraud_user_deactivate_period = GrpcAlgs::pack_time(
        config->global_params.fraud_user_deactivate_period);
      result->cost_limit = GrpcAlgs::pack_decimal(config->global_params.cost_limit);
      result->google_publisher_account_id = config->global_params.google_publisher_account_id;
      result->global_params_timestamp = GrpcAlgs::pack_time(config->global_params.timestamp);

      fill_object_update_sequences_(
        result->activate_creative_options,
        &result->delete_creative_options,
        config->creative_options,
        OptionFillAdapter(request_timestamp, settings.portion, settings.portions_number),
        "creative_options");

      fill_app_formats_(request_timestamp, config, settings, *result);
      fill_sizes_(request_timestamp, config, settings, *result);
      fill_countries_(request_timestamp, config, settings, *result);

      fill_tags_(request_timestamp, config, settings, *result);

      if (!settings.provide_only_tags)
      {
        fill_adv_actions_(request_timestamp, config, settings, *result);

        fill_creative_categories_(request_timestamp, config, settings, *result);

        fill_simple_channels_(
          request_timestamp,
          config,
          countries,
          settings.channel_statuses.c_str(),
          settings,
          *result);

        result->geo_channels_timestamp = GrpcAlgs::pack_time(config->geo_channels->max_stamp());

        if (GrpcAlgs::unpack_time(settings.geo_channels_timestamp) <
           config->geo_channels->max_stamp())
        {
          fill_object_update_sequences_(
            result->activate_geo_channels,
            &result->delete_geo_channels,
            *config->geo_channels,
            GeoChannelFillAdapter(
              request_timestamp,
              settings.portion,
              settings.portions_number,
              &countries),
            "geo_channels");
        }

        fill_deleted_sequence(
          result->delete_geo_coord_channels,
          request_timestamp,
          config->geo_coord_channels.inactive(),
          settings);

        fill_object_update_sequences_(
          result->activate_geo_coord_channels,
          &(result->delete_geo_coord_channels),
          config->geo_coord_channels,
          GeoCoordChannelFillAdapter(request_timestamp, settings.portion, settings.portions_number),
          "geo_coord_channels");

        fill_block_channels_(request_timestamp, config, settings, *result);

        fill_expression_channels_(
          request_timestamp, config, countries, settings.channel_statuses.c_str(),
          settings, *result);

        fill_category_channels_(request_timestamp, config, settings, *result);

        fill_campaigns_(request_timestamp, config, countries, settings, *result);

        fill_ecpms_(request_timestamp, config, campaign_statuses.c_str(), settings, *result);

        fill_accounts_(request_timestamp, config, settings, *result);

        fill_colocations_(request_timestamp, config, settings, *result);

        fill_freq_caps_(request_timestamp, config, settings, *result);

        fill_creative_templates_(request_timestamp, config, settings, *result);

        fill_sites_(request_timestamp, config, settings, *result);

        fill_currencies_(request_timestamp, config, settings, *result);

        fill_campaign_keywords_(request_timestamp, config, settings, *result);

        fill_behavioral_parameters_(request_timestamp, config, settings, *result);

        fill_fraud_conditions_(
          request_timestamp, config, settings,
          result->fraud_conditions,
          &result->deleted_fraud_conditions);

        fill_search_enginies_(
          request_timestamp,
          config,
          settings,
          result->search_engines,
          &result->deleted_search_engines);

        fill_contracts_(request_timestamp, config, settings, *result);

        fill_object_update_sequences_(
          result->web_browsers,
          &result->deleted_web_browsers,
          config->web_browsers,
          WebBrowserFillAdapter(request_timestamp, settings.portion, settings.portions_number),
          "web_browsers");

        fill_object_update_sequences_(
          result->platforms,
          &result->deleted_platforms,
          config->platforms,
          PlatformFillAdapter(request_timestamp, settings.portion, settings.portions_number),
          "platforms");

        fill_object_update_sequences_(
          result->web_operations,
          &result->delete_web_operations,
          config->web_operations,
          WebOperationFillAdapter(request_timestamp, settings.portion, settings.portions_number),
          "web operations");
      }

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->stream(Logging::Logger::TRACE,
          Aspect::CAMPAIGN_SERVER) << FUN << ": from get_config.";
      }

      try
      {
        /* log campaign update */
        if (settings.colo_id != 0)
        {
          CampaignServerLogger::ConfigUpdateInfo config_update_info;
          config_update_info.colo_id = settings.colo_id;
          config_update_info.time = Generics::Time::get_time_of_day();
          config_update_info.version = settings.version;

          campaign_server_logger_->process_config_update(config_update_info);
        }
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::ERROR, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-164") << FUN <<
          ": Can't log config updating stat: eh::Exception caught: " << ex.what();
      }

      return result.release();
    }
    catch(const AdServer::CampaignSvcs::CampaignServer::NotReady&)
    {
      throw;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();

      logger_->log(ostr.str(), Logging::Logger::ERROR, Aspect::CAMPAIGN_SERVER, "ADS-IMPL-166");
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0; // never reach
  }

  EcpmSeq*
  CampaignServerBaseImpl::get_ecpms(const AdServer::CampaignSvcs::TimestampInfo& request_ts)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::get_ecpms()";

    try
    {
      TimestampValue request_timestamp = GrpcAlgs::unpack_time(request_ts);

      CampaignConfig_var config = campaign_config();

      std::unique_ptr<EcpmSeq> result(new EcpmSeq());

      if (config.in() != 0)
      {
        result->resize(config->ecpms.active().size());

        unsigned long real_size = 0;

        for (EcpmMap::ActiveMap::iterator it = config->ecpms.active().begin();
             it != config->ecpms.active().end(); ++it)
        {
          if (it->second->timestamp > request_timestamp)
          {
            CampaignEcpmInfo& ecpm = (*result)[real_size++];
            ecpm.ccg_id = it->first;
            ecpm.ecpm = GrpcAlgs::pack_decimal(it->second->ecpm);
            ecpm.ctr = GrpcAlgs::pack_decimal(it->second->ctr);
            ecpm.timestamp = GrpcAlgs::pack_time(it->second->timestamp);
          }
        }
        result->resize(real_size);
      }

      return result.release();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught:" << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0; // never reach
  }

  void
  CampaignServerBaseImpl::fill_simple_channels_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const StringSet& countries,
    const char* channel_statuses,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::fill_simple_channels_()";

    try
    {
      AdServer::CampaignSvcs::DeletedIdSeq& deleted_result_seq =
        update_info.deleted_simple_channels;

      fill_deleted_sequence(
        deleted_result_seq,
        request_timestamp,
        campaign_config->simple_channels.inactive(),
        settings);

      AdServer::CampaignSvcs::SimpleChannelKeySeq& result_seq = update_info.simple_channels;

      std::uint32_t res_len = count_updated_instances_(
        campaign_config->simple_channels.active(),
        request_timestamp,
        settings.portion,
        settings.portions_number,
        AdServer::Commons::PointerTimestampOp<SimpleChannelDef_var>());

      result_seq.resize(res_len);

      std::uint32_t deleted_simple_channels_i = deleted_result_seq.size();
      deleted_result_seq.resize(deleted_simple_channels_i + res_len);

      std::uint32_t simple_channel_i = 0;
      for (SimpleChannelMap::ActiveMap::const_iterator it =
            campaign_config->simple_channels.active().begin();
          it != campaign_config->simple_channels.active().end(); ++it)
      {
        if (it->second->timestamp > request_timestamp &&
           it->first % settings.portions_number == settings.portion)
        {
          const SimpleChannelDef& s_channel = *it->second;

          if ((countries.empty() || countries.find(s_channel.country) != countries.end()) &&
             (channel_statuses[0] == 0 || strchr(channel_statuses, s_channel.status) != 0))
          {
            // push to active simple channels
            pack_simple_channel(result_seq[simple_channel_i++], s_channel);
          }
          else
          {
            // push to deleted simple channels
            AdServer::CampaignSvcs::DeletedIdInfo& del_ch =
              deleted_result_seq[deleted_simple_channels_i];

            del_ch.timestamp = GrpcAlgs::pack_time(it->second->timestamp);
            del_ch.id = it->first;

            ++deleted_simple_channels_i;
          }
        }
      }

      deleted_result_seq.resize(deleted_simple_channels_i);
      result_seq.resize(simple_channel_i);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      throw Exception(ostr.str());
    }
  }

  void
  CampaignServerBaseImpl::fill_block_channels_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::fill_block_channels_()";

    try
    {
      fill_deleted_sequence(
        update_info.delete_block_channels,
        request_timestamp,
        campaign_config->block_channels.inactive(),
        settings);

      AdServer::CampaignSvcs::BlockChannelSeq& result_seq = update_info.activate_block_channels;

      std::uint32_t res_len = count_updated_instances_(
        campaign_config->block_channels.active(),
        request_timestamp,
        settings.portion,
        settings.portions_number,
        AdServer::Commons::PointerTimestampOp<BlockChannelDef_var>());

      result_seq.resize(res_len);

      std::uint32_t block_channel_i = 0;
      for (BlockChannelMap::ActiveMap::const_iterator it =
            campaign_config->block_channels.active().begin();
          it != campaign_config->block_channels.active().end(); ++it)
      {
        if (it->second->timestamp > request_timestamp &&
           it->first % settings.portions_number == settings.portion)
        {
          BlockChannelInfo& res_block_channel = result_seq[block_channel_i++];
          res_block_channel.channel_id = it->first;
          res_block_channel.size_id = it->second->size_id;
          res_block_channel.timestamp = GrpcAlgs::pack_time(it->second->timestamp);
        }
      }

      result_seq.resize(block_channel_i);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      throw Exception(ostr.str());
    }
  }

  void
  CampaignServerBaseImpl::fill_expression_channels_(
    const TimestampValue& request_timestamp,
    const CampaignConfig* campaign_config,
    const StringSet& countries,
    const char* channel_statuses,
    const CampaignGetConfigSettings& settings,
    CampaignConfigUpdateInfo& update_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignServerImpl::fill_expression_channels_()";

    try
    {
      fill_deleted_sequence(
        update_info.deleted_expression_channels,
        request_timestamp,
        campaign_config->expression_channels.inactive(),
        settings);

      std::uint32_t res_len = 0;
      std::uint32_t del_res_len = 0;

      for (ChannelMap::ActiveMap::const_iterator ch_it =
            campaign_config->expression_channels.active().begin();
          ch_it != campaign_config->expression_channels.active().end();
          ++ch_it)
      {
        const ChannelParams& ch_params = ch_it->second->params();

        if (ch_params.timestamp > request_timestamp &&
            ch_it->first % settings.portions_number == settings.portion)
        {
          if ((countries.empty() ||
                ch_params.country.empty() ||
                countries.find(ch_params.country) != countries.end()) &&
             (channel_statuses[0] == 0 || strchr(channel_statuses, ch_params.status) != 0))
          {
            ++res_len;
          }
          else if (!settings.no_deleted)
          {
            ++del_res_len;
          }
        }
      }

      std::uint32_t expression_channels_i = update_info.expression_channels.size();
      std::uint32_t deleted_channels_i = update_info.deleted_expression_channels.size();

      update_info.expression_channels.resize(expression_channels_i + res_len);
      update_info.deleted_expression_channels.resize(deleted_channels_i + del_res_len);

      for (ChannelMap::ActiveMap::const_iterator ch_it =
            campaign_config->expression_channels.active().begin();
          ch_it != campaign_config->expression_channels.active().end();
          ++ch_it)
      {
        const ChannelParams& ch_params = ch_it->second->params();

        if (ch_params.timestamp > request_timestamp &&
            ch_it->first % settings.portions_number == settings.portion)
        {
          if ((countries.empty() ||
                ch_params.country.empty() ||
                countries.find(ch_params.country) != countries.end()) &&
             (channel_statuses[0] == 0 || strchr(channel_statuses, ch_params.status) != 0))
          {
            assert(expression_channels_i < res_len);
            pack_non_linked_expression_channel(
              update_info.expression_channels[expression_channels_i],
              ch_it->second);
            ++expression_channels_i;
          }
          else if (!settings.no_deleted)
          {
            assert(deleted_channels_i < update_info.deleted_expression_channels.size());
            // push to deleted channels
            AdServer::CampaignSvcs::DeletedIdInfo& del_ch =
              update_info.deleted_expression_channels[deleted_channels_i];

            del_ch.id = ch_it->first;
            del_ch.timestamp = GrpcAlgs::pack_time(ch_params.timestamp);

            ++deleted_channels_i;
          }
        }
      }

      update_info.deleted_expression_channels.resize(deleted_channels_i);
      update_info.expression_channels.resize(expression_channels_i);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      throw Exception(ostr.str());
    }
  }

  AdServer::CampaignSvcs::ChannelServerChannelAnswer*
  CampaignServerBaseImpl::chsv_simple_channels(
    const AdServer::CampaignSvcs::CampaignServer::GetSimpleChannelsInfo& settings)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    try
    {
      CampaignConfig_var config = campaign_config();
      if (config.in() == 0)
      {
        throw CampaignServer::NotReady("Campaign configuration isn't loaded");
      }
      auto res = std::make_unique<AdServer::CampaignSvcs::ChannelServerChannelAnswer>();

      typedef std::vector<AdServer::CampaignSvcs::BehaveInfo> BehaveInformation;
      typedef std::map<unsigned long, BehaveInformation> BehaveForChannel;

      BehaveForChannel info;

      std::uint32_t res_len = 0;
      for (SimpleChannelMap::ActiveMap::const_iterator it =
            config->simple_channels.active().begin();
          it != config->simple_channels.active().end(); ++it)
      {
        if (it->first % settings.portions_number == settings.portion &&
           (settings.channel_statuses[0] == 0 ||
            strchr(settings.channel_statuses.c_str(), it->second->status) != 0))
        {
          const BehavioralParameterListDef* channel_bps = 0;
          if (it->second->behav_param_list_id)
          {
            BehavioralParameterMap::ActiveMap::const_iterator bp_it =
              config->behav_param_lists.active().find(it->second->behav_param_list_id);
            if (bp_it != config->behav_param_lists.active().end())
            {
              channel_bps = bp_it->second.in();
            }
          }
          else if (!it->second->str_behav_param_list_id.empty())
          {
            BehavioralParameterKeyMap::ActiveMap::const_iterator bp_it =
              config->str_behav_param_lists.active().find(it->second->str_behav_param_list_id);
            if (bp_it != config->str_behav_param_lists.active().end())
            {
              channel_bps = bp_it->second.in();
            }
          }

          if (channel_bps && !channel_bps->behave_params.empty())
          {
            BehaveInformation& ch_info = info[it->second->channel_id];
            ch_info.reserve(channel_bps->behave_params.size());
            for (BehavioralParameterListDef::BehavioralParameterList::
                  const_iterator bp_it = channel_bps->behave_params.begin();
                bp_it != channel_bps->behave_params.end(); ++bp_it)
            {
              ch_info.push_back(AdServer::CampaignSvcs::BehaveInfo{
                bp_it->time_from == 0 && bp_it->min_visits == 1,
                bp_it->trigger_type,
                bp_it->weight});
            }
          }
          ++res_len;
        }
      }
      res->simple_channels.resize(res_len);

      std::uint32_t i = 0;
      for (SimpleChannelMap::ActiveMap::const_iterator it =
            config->simple_channels.active().begin();
          it != config->simple_channels.active().end(); ++it)
      {
        if (it->first % settings.portions_number == settings.portion &&
           (settings.channel_statuses[0] == 0 ||
            strchr(settings.channel_statuses.c_str(), it->second->status) != 0))
        {
          const SimpleChannelDef& in = *it->second;
          AdServer::CampaignSvcs::CSSimpleChannel& ch_inf = res->simple_channels[i++];
          ch_inf.channel_id = in.channel_id;
          ch_inf.country_code = in.country;
          ch_inf.status = in.status;

          ChannelMap::ActiveMap::const_iterator ch_it =
            config->expression_channels.active().find(it->first);

          if (ch_it != config->expression_channels.active().end())
          {
            const ChannelParams& params = ch_it->second->params();
            ch_inf.language = params.common_params->language;
            ch_inf.channel_type = params.type;
          }
          else
          {
            ch_inf.channel_type = 'W';
          }
          BehaveForChannel::iterator bp_info_it = info.find(in.channel_id);
          if (bp_info_it != info.end())
          {
            ch_inf.behave_info.resize(bp_info_it->second.size());
            std::copy(
              bp_info_it->second.begin(),
              bp_info_it->second.end(),
              ch_inf.behave_info.data());
          }
        }
      }
      res->cost_limit = GrpcAlgs::pack_decimal(config->global_params.cost_limit);

      return res.release();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0; // never reach
  }

  AdServer::CampaignSvcs::BriefSimpleChannelAnswer*
  CampaignServerBaseImpl::brief_simple_channels(
    const AdServer::CampaignSvcs::CampaignServer::GetSimpleChannelsInfo& settings)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::brief_simple_channels";
    try
    {
      CampaignConfig_var config = campaign_config();
      if (config.in() == 0)
      {
        throw CampaignServer::NotReady("Campaign configuration isn't loaded");
      }
      auto res = std::make_unique<AdServer::CampaignSvcs::BriefSimpleChannelAnswer>();

      std::uint32_t res_len = 0;

      for (SimpleChannelMap::ActiveMap::const_iterator it =
            config->simple_channels.active().begin();
          it != config->simple_channels.active().end(); ++it)
      {
        if (it->first % settings.portions_number == settings.portion &&
          (it->second->behav_param_list_id != 0 ||
           !it->second->str_behav_param_list_id.empty()) && //filter channels without BH
           (settings.channel_statuses[0] == 0 ||
            strchr(settings.channel_statuses.c_str(), it->second->status) != 0))
        {
          ++res_len;
        }
      }

      res->simple_channels.resize(res_len);
      std::uint32_t i = 0;

      for (SimpleChannelMap::ActiveMap::const_iterator it =
            config->simple_channels.active().begin();
          it != config->simple_channels.active().end(); ++it)
      {
        if (it->first % settings.portions_number == settings.portion &&
          (it->second->behav_param_list_id != 0 ||
           !it->second->str_behav_param_list_id.empty()) && //filter channels without BH
           (settings.channel_statuses[0] == 0 ||
            strchr(settings.channel_statuses.c_str(), it->second->status) != 0))
        {
          pack_brief_simple_channel(res->simple_channels[i], *it->second);

          ChannelMap::ActiveMap::const_iterator ch_it =
            config->expression_channels.active().find(it->first);

          if (ch_it != config->expression_channels.active().end())
          {
            const ChannelParams& params = ch_it->second->params();
            res->simple_channels[i].discover = params.discover_params.in();
          }
          else
          {
            res->simple_channels[i].discover = false;
          }

          ++i;
        }
      }
      res->simple_channels.resize(i);

      fill_brief_bp_parameters_(
        Generics::Time::ZERO,
        config,
        settings.portion,
        settings.portions_number,
        res->behav_params,
        res->key_behav_params);

      if (colo_id_)
      {
        bool colo_found = false;
        ColocationMap::ActiveMap::const_iterator c_it = config->colocations.active().find(colo_id_);

        if (c_it != config->colocations.active().end())
        {
          AccountMap::ActiveMap::const_iterator a_it =
            config->accounts.active().find(c_it->second->account_id);

          if (a_it != config->accounts.active().end())
          {
            res->timezone_offset = GrpcAlgs::pack_time(a_it->second->time_offset);
            colo_found = true;
          }
        }

        if (!colo_found)
        {
          throw Exception("Cannot find current colo_id in colocations.");
        }
      }
      else
      {
        res->timezone_offset = GrpcAlgs::pack_time(Generics::Time::ZERO);
      }

      res->master_stamp = GrpcAlgs::pack_time(config->master_stamp);

      return res.release();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }

    return 0; // never reach
  }

  AdServer::CampaignSvcs::SimpleChannelAnswer*
  CampaignServerBaseImpl::simple_channels(
    const AdServer::CampaignSvcs::CampaignServer::GetSimpleChannelsInfo& settings)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::simple_channels";
    try
    {
      CampaignConfig_var config = campaign_config();
      if (config.in() == 0)
      {
        throw CampaignServer::NotReady("Campaign configuration isn't loaded");
      }
      auto res = std::make_unique<AdServer::CampaignSvcs::SimpleChannelAnswer>();
/*
      std::uint32_t res_len = count_updated_instances_(
        config->simple_channels.active(),
        Generics::Time::ZERO,
        settings.portion,
        settings.portions_number,
        AdServer::Commons::PointerTimestampOp<SimpleChannelDef_var>(),
        settings.channel_statuses.c_str());
*/

      std::uint32_t res_len = 0;

      for (SimpleChannelMap::ActiveMap::const_iterator it =
            config->simple_channels.active().begin();
          it != config->simple_channels.active().end(); ++it)
      {
        if (it->first % settings.portions_number == settings.portion &&
           (settings.channel_statuses[0] == 0 ||
            strchr(settings.channel_statuses.c_str(), it->second->status) != 0))
        {
          ++res_len;
        }
      }

      res->simple_channels.resize(res_len);
      std::uint32_t i = 0;

      for (SimpleChannelMap::ActiveMap::const_iterator it =
            config->simple_channels.active().begin();
          it != config->simple_channels.active().end(); ++it)
      {
        if (it->first % settings.portions_number == settings.portion &&
           (settings.channel_statuses[0] == 0 ||
            strchr(settings.channel_statuses.c_str(), it->second->status) != 0))
        {
          pack_simple_channel(res->simple_channels[i], *it->second);

          ChannelMap::ActiveMap::const_iterator ch_it =
            config->expression_channels.active().find(it->first);

          if (ch_it != config->expression_channels.active().end())
          {
            const ChannelParams& params = ch_it->second->params();
            res->simple_channels[i].discover = params.discover_params.in();
            res->simple_channels[i].language = params.common_params->language;
          }
          else
          {
            res->simple_channels[i].discover = false;
          }

          ++i;
        }
      }
      res->simple_channels.resize(i);

      fill_bp_parameters_(
        Generics::Time::ZERO,
        config,
        settings.portion,
        settings.portions_number,
        res->behav_params,
        res->key_behav_params);

      if (colo_id_)
      {
        bool colo_found = false;
        ColocationMap::ActiveMap::const_iterator c_it = config->colocations.active().find(colo_id_);

        if (c_it != config->colocations.active().end())
        {
          AccountMap::ActiveMap::const_iterator a_it =
            config->accounts.active().find(c_it->second->account_id);

          if (a_it != config->accounts.active().end())
          {
            res->timezone_offset = GrpcAlgs::pack_time(a_it->second->time_offset);
            colo_found = true;
          }
        }

        if (!colo_found)
        {
          throw Exception("Cannot find current colo_id in colocations.");
        }
      }
      else
      {
        res->timezone_offset = GrpcAlgs::pack_time(Generics::Time::ZERO);
      }

      res->master_stamp = GrpcAlgs::pack_time(config->master_stamp);
      res->cost_limit = GrpcAlgs::pack_decimal(config->global_params.cost_limit);

      return res.release();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0; // never reach
  }

  ExpressionChannelsInfo*
  CampaignServerBaseImpl::get_expression_channels(
    const AdServer::CampaignSvcs::CampaignServer::
      GetExpressionChannelsInfo& request_settings)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::get_expression_channels()";

    typedef std::set<unsigned long> CCGIdSet;
    typedef std::map<unsigned long, CCGIdSet> ChannelCCGMap;

    try
    {
      CampaignConfig_var config = campaign_config();

      if (config.in() == 0)
      {
        throw CampaignServer::NotReady("Campaign configuration isn't loaded");
      }

      auto result = std::make_unique<AdServer::CampaignSvcs::ExpressionChannelsInfo>();
      Generics::Time request_timestamp = GrpcAlgs::unpack_time(request_settings.timestamp);
      if (request_timestamp < config->first_load_stamp)
      {
        request_timestamp = Generics::Time::ZERO;
      }

      std::uint32_t res_size = 0;
      for (ChannelMap::ActiveMap::const_iterator ch_it =
            config->expression_channels.active().begin();
          ch_it != config->expression_channels.active().end();
          ++ch_it)
      {
        if (ch_it->first % request_settings.portions_number ==
             request_settings.portion &&
           (request_settings.channel_types[0] == 0 ||
             strchr(request_settings.channel_types.c_str(), ch_it->second->params().type) != 0) &&
           strchr(request_settings.channel_statuses.c_str(), ch_it->second->params().status) != 0)
        {
          ++res_size;
        }
      }

      if ((request_settings.channel_types[0] == 0 ||
          strchr(request_settings.channel_types.c_str(), 'B') != 0) &&
         strchr(request_settings.channel_statuses.c_str(), 'A') != 0)
      {
        for (PlatformMap::ActiveMap::const_iterator pl_it = config->platforms.active().begin();
            pl_it != config->platforms.active().end(); ++pl_it)
        {
          if (pl_it->first % request_settings.portions_number == request_settings.portion)
          {
            ++res_size;
          }
        }
      }

      AdServer::CampaignSvcs::ExpressionChannelSeq& result_channels = result->expression_channels;

      result_channels.resize(res_size);
      if (request_settings.provide_overlap_channel_ids)
      {
        result->overlap_channel_ids.resize(res_size);
      }

      std::uint32_t ch_i = 0;
      std::uint32_t overlap_channel_i = 0;

      for (ChannelMap::ActiveMap::const_iterator ch_it =
            config->expression_channels.active().begin();
          ch_it != config->expression_channels.active().end();
          ++ch_it)
      {
        if (ch_it->first % request_settings.portions_number ==
             request_settings.portion &&
           (request_settings.channel_types[0] == 0 ||
            strchr(request_settings.channel_types.c_str(), ch_it->second->params().type) != 0) &&
           strchr(request_settings.channel_statuses.c_str(), ch_it->second->params().status) != 0)
        {
          assert(ch_it->first == ch_it->second->params().channel_id);
          assert(ch_i < res_size);
          pack_non_linked_expression_channel(result_channels[ch_i++], ch_it->second);

          // only public, linked to internal account will be overlapped
          if (request_settings.provide_overlap_channel_ids &&
             ch_it->second->params().common_params.in() &&
             ch_it->second->params().common_params->is_public)
          {
            AccountMap::ActiveMap::const_iterator it =
              config->accounts.active().find(ch_it->second->params().common_params->account_id);
            if (it != config->accounts.active().end() && it->second->internal_account_id == 0)
            {
              result->overlap_channel_ids[overlap_channel_i++] = ch_it->second->params().channel_id;
            }
          }
        }
      }

      result->overlap_channel_ids.resize(overlap_channel_i);

      if ((request_settings.channel_types[0] == 0 ||
          strchr(request_settings.channel_types.c_str(), 'B') != 0) &&
         strchr(request_settings.channel_statuses.c_str(), 'A') != 0)
      {
        for (PlatformMap::ActiveMap::const_iterator pl_it = config->platforms.active().begin();
            pl_it != config->platforms.active().end(); ++pl_it)
        {
          if (pl_it->first % request_settings.portions_number == request_settings.portion)
          {
            assert(ch_i < res_size);
            pack_platform_expression_channel(
              result_channels[ch_i++],
              pl_it->first,
              pl_it->second->timestamp);
          }
        }
      }

      assert(ch_i == res_size);

      if (request_settings.provide_ccg_links)
      {
        ChannelCCGMap channel_ccgs;

        // fill expression channel usage in ccgs
        for (CampaignMap::ActiveMap::const_iterator ccg_it = config->campaigns.active().begin();
            ccg_it != config->campaigns.active().end(); ++ccg_it)
        {
          ChannelIdSet ccg_channels;
          ccg_it->second->stat_expression.all_channels(ccg_channels);
          for (ChannelIdSet::const_iterator ch_it = ccg_channels.begin();
              ch_it != ccg_channels.end(); ++ch_it)
          {
            if (*ch_it % request_settings.portions_number == request_settings.portion)
            {
              channel_ccgs[*ch_it].insert(ccg_it->first);
            }
          }
        }

        result->expression_channel_ccgs.resize(channel_ccgs.size());
        std::uint32_t ch_i = 0;
        for (ChannelCCGMap::const_iterator ch_ccg_it = channel_ccgs.begin();
            ch_ccg_it != channel_ccgs.end(); ++ch_ccg_it, ++ch_i)
        {
          result->expression_channel_ccgs[ch_i].channel_id = ch_ccg_it->first;
          result->expression_channel_ccgs[ch_i].ccgs.assign(
            ch_ccg_it->second.begin(), ch_ccg_it->second.end());
        }
      }

      if (request_settings.provide_channel_triggers)
      {
        std::uint32_t res_size = 0;
        for (SimpleChannelMap::ActiveMap::const_iterator ch_it =
              config->simple_channels.active().begin();
            ch_it != config->simple_channels.active().end();
            ++ch_it)
        {
          if (ch_it->first % request_settings.portions_number ==
               request_settings.portion &&
             ch_it->second->match_params.in() &&
             ch_it->second->timestamp > request_timestamp)
          {
            ++res_size;
          }
        }

        result->activate_channel_triggers.resize(res_size);

        std::uint32_t res_i = 0;
        for (SimpleChannelMap::ActiveMap::const_iterator ch_it =
              config->simple_channels.active().begin();
            ch_it != config->simple_channels.active().end();
            ++ch_it)
        {
          if (ch_it->first % request_settings.portions_number ==
               request_settings.portion &&
             ch_it->second->match_params.in() &&
             ch_it->second->timestamp > request_timestamp)
          {
            const BehavioralParameterListDef* channel_bps = 0;
            if (!ch_it->second->str_behav_param_list_id.empty())
            {
              BehavioralParameterKeyMap::ActiveMap::const_iterator bp_it =
                config->str_behav_param_lists.active().find(ch_it->second->str_behav_param_list_id);
              if (bp_it != config->str_behav_param_lists.active().end())
              {
                channel_bps = bp_it->second.in();
              }
            }
            else
            {
              BehavioralParameterMap::ActiveMap::const_iterator bp_it =
                config->behav_param_lists.active().find(ch_it->second->behav_param_list_id);
              if (bp_it != config->behav_param_lists.active().end())
              {
                channel_bps = bp_it->second.in();
              }
            }

            if (channel_bps)
            {
              CampaignSvcs::ChannelTriggersInfo& cht_info =
                result->activate_channel_triggers[res_i];
              cht_info.page_min_visits = 0;
              cht_info.search_min_visits = 0;
              cht_info.url_min_visits = 0;
              cht_info.url_keyword_min_visits = 0;
              cht_info.page_time_to = 0;
              cht_info.search_time_to = 0;
              cht_info.url_time_to = 0;
              cht_info.url_keyword_time_to = 0;

              for (BehavioralParameterListDef::BehavioralParameterList::
                    const_iterator bp_it = channel_bps->behave_params.begin();
                  bp_it != channel_bps->behave_params.end(); ++bp_it)
              {
                if (bp_it->trigger_type == 'P')
                {
                  cht_info.page_min_visits = std::max(cht_info.page_min_visits, bp_it->min_visits);
                  cht_info.page_time_to = std::max(cht_info.page_time_to, bp_it->time_to);
                }
                else if (bp_it->trigger_type == 'S')
                {
                  cht_info.search_min_visits = std::max(
                    cht_info.search_min_visits, bp_it->min_visits);
                  cht_info.search_time_to = std::max(cht_info.search_time_to, bp_it->time_to);
                }
                else if (bp_it->trigger_type == 'U')
                {
                  cht_info.url_min_visits = std::max(cht_info.url_min_visits, bp_it->min_visits);
                  cht_info.url_time_to = std::max(cht_info.url_time_to, bp_it->time_to);
                }
                else if (bp_it->trigger_type == 'R')
                {
                  cht_info.url_keyword_min_visits = std::max(
                    cht_info.url_keyword_min_visits, bp_it->min_visits);
                  cht_info.url_keyword_time_to = std::max(
                    cht_info.url_keyword_time_to, bp_it->time_to);
                }
              }

              cht_info.channel_id = ch_it->first;
              cht_info.page_triggers.assign(
                ch_it->second->match_params->page_triggers.begin(),
                ch_it->second->match_params->page_triggers.end());
              cht_info.search_triggers.assign(
                ch_it->second->match_params->search_triggers.begin(),
                ch_it->second->match_params->search_triggers.end());
              cht_info.url_triggers.assign(
                ch_it->second->match_params->url_triggers.begin(),
                ch_it->second->match_params->url_triggers.end());
              cht_info.url_keyword_triggers.assign(
                ch_it->second->match_params->url_keyword_triggers.begin(),
                ch_it->second->match_params->url_keyword_triggers.end());
              ++res_i;
            }
          }
        }

        fill_deleted_sequence(
          result->delete_simple_channels,
          request_timestamp,
          config->simple_channels.inactive(),
          request_settings.portion,
          request_settings.portions_number);
      }

      // fill actions
      result->activate_actions.resize(config->adv_actions.active().size());
      std::uint32_t res_act_i = 0;
      for (auto act_it = config->adv_actions.active().begin();
        act_it != config->adv_actions.active().end();
        ++act_it)
      {
        if (act_it->first % request_settings.portions_number ==
             request_settings.portion &&
           act_it->second->timestamp > request_timestamp)
        {
          auto& act_info = result->activate_actions[res_act_i];
          act_info.action_id = act_it->first;
          act_info.timestamp = GrpcAlgs::pack_time(act_it->second->timestamp);
          act_info.ccg_ids.assign(act_it->second->ccg_ids.begin(), act_it->second->ccg_ids.end());

          ++res_act_i;
        }
      }

      result->activate_actions.resize(res_act_i);

      fill_deleted_sequence(
        result->delete_actions,
        request_timestamp,
        config->adv_actions.inactive(),
        request_settings.portion,
        request_settings.portions_number);

      result->timestamp = GrpcAlgs::pack_time(config->master_stamp);
      result->first_load_stamp = GrpcAlgs::pack_time(config->first_load_stamp);

      return result.release();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0; // never reach
  }

  AdServer::CampaignSvcs::CampaignServer::PassbackInfo*
  CampaignServerBaseImpl::get_tag_passback(std::uint32_t tag_id, const char* app_format)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::get_tag_passback()";

    try
    {
      CampaignConfig_var config = campaign_config();

      if (config.in() == 0)
      {
        throw CampaignServer::NotReady("Campaign configuration isn't loaded");
      }

      auto passback_info = std::make_unique<AdServer::CampaignSvcs::CampaignServer::PassbackInfo>();

      passback_info->active = false;

      TagMap::ActiveMap::const_iterator it = config->tags.active().find(tag_id);
      if (it != config->tags.active().end())
      {
        passback_info->active = true;
        passback_info->passback_type = it->second->passback_type;
        passback_info->passback_url = it->second->passback;

        if (!it->second->sizes.empty())
        {
          SizeMap::ActiveMap::const_iterator size_it =
            config->sizes.active().find(it->second->sizes.begin()->first);
          if (size_it != config->sizes.active().end())
          {
            passback_info->width = size_it->second->width;
            passback_info->height = size_it->second->height;
            passback_info->size = size_it->second->protocol_name;
          }
        }

        for (OptionValueMap::const_iterator opt_it = it->second->passback_tokens.begin();
            opt_it != it->second->passback_tokens.end(); ++opt_it)
        {
          CreativeOptionMap::ActiveMap::const_iterator copt_it =
            config->creative_options.active().find(opt_it->first);
          if (copt_it != config->creative_options.active().end() &&
             copt_it->second->token == "PASSBACK_CODE")
          {
            passback_info->passback_code = opt_it->second;
            break;
          }
        }
      }

      AppFormatMap::ActiveMap::const_iterator app_format_it =
        config->app_formats.active().find(app_format);
      if (app_format_it != config->app_formats.active().end())
      {
        passback_info->mime_format = app_format_it->second->mime_format;
      }

      return passback_info.release();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0; // never reach
  }

  AdServer::CampaignSvcs::FraudConditionConfig*
  CampaignServerBaseImpl::fraud_conditions()
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::fraud_conditions()";

    try
    {
      CampaignConfig_var config = campaign_config();
      if (config.in() == 0)
      {
        throw CampaignServer::NotReady("Campaign configuration isn't loaded");
      }

      auto res = std::make_unique<AdServer::CampaignSvcs::FraudConditionConfig>();

      CampaignGetConfigSettings each_settings;
      each_settings.portion = 0;
      each_settings.portions_number = 1;
      fill_fraud_conditions_(Generics::Time::ZERO, config, each_settings, res->rules, 0);

      res->deactivate_period = GrpcAlgs::pack_time(
        config->global_params.fraud_user_deactivate_period);

      return res.release();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0;
  }

  AdServer::CampaignSvcs::FreqCapConfigInfo*
  CampaignServerBaseImpl::freq_caps()
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::freq_caps()";

    try
    {
      CampaignConfig_var config = campaign_config();
      if (config.in() == 0)
      {
        throw CampaignServer::NotReady("Campaign configuration isn't loaded");
      }

      auto res = std::make_unique<AdServer::CampaignSvcs::FreqCapConfigInfo>();

      fill_active_freq_caps_(
        res->freq_caps,
        Generics::Time::ZERO,
        config->freq_caps.active(),
        0,
        1);

      fill_active_campaign_ids_(res->campaign_ids, config->campaigns.active());

      return res.release();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0;
  }

  ColocationFlagsSeq*
  CampaignServerBaseImpl::get_colocation_flags()
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::get_colocation_flags()";

    try
    {
      CampaignConfig_var config = campaign_config();

      if (!config)
      {
        throw AdServer::CampaignSvcs::CampaignServer::NotReady(
          "Campaign configuration isn't loaded");
      }

      std::unique_ptr<ColocationFlagsSeq> result(new ColocationFlagsSeq());
      result->resize(config->colocations.active().size());
      unsigned long real_size = 0;

      for (ColocationMap::ActiveMap::const_iterator it = config->colocations.active().begin();
        it != config->colocations.active().end(); ++it)
      {
        ColocationFlags& colo = (*result)[real_size++];
        colo.colo_id = it->first;
        colo.flags = it->second->ad_serving;
        colo.hid_profile = it->second->hid_profile;
      }

      result->resize(real_size);
      return result.release();
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0; // never reach
  }

  AdServer::CampaignSvcs::CampaignServer::ColocationPropInfo*
  CampaignServerBaseImpl::get_colocation_prop(std::uint32_t colo_id)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::get_colocation_prop()";

    try
    {
      CampaignConfig_var config = campaign_config();

      if (!config)
      {
        throw AdServer::CampaignSvcs::CampaignServer::NotReady(
          "Campaign configuration isn't loaded");
      }

      auto result = std::make_unique<AdServer::CampaignSvcs::CampaignServer::ColocationPropInfo>();
      result->found = false;
      const ColocationMap::ActiveMap::const_iterator colo_it =
        config->colocations.active().find(colo_id);

      if (colo_it != config->colocations.active().end())
      {
        const AccountMap::ActiveMap::const_iterator acc_it =
          config->accounts.active().find(colo_it->second->account_id);

        if (acc_it != config->accounts.active().end())
        {
          result->found = true;
          result->time_offset = GrpcAlgs::pack_time(acc_it->second->time_offset);
        }
      }

      return result.release();
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }

    return nullptr;  // never reach
  }

  AdServer::CampaignSvcs::CampaignServer::DeliveryLimitConfigInfo*
  CampaignServerBaseImpl::get_delivery_limit_config()
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::get_delivery_limit_config()";

    try
    {
      CampaignConfig_var config = campaign_config();

      if (!config)
      {
        throw AdServer::CampaignSvcs::CampaignServer::NotReady(
          "Campaign configuration isn't loaded");
      }

      auto result = std::make_unique<CampaignServer::DeliveryLimitConfigInfo>();

      result->accounts.resize(config->accounts.active().size());
      std::uint32_t res_acc_i = 0;
      for (AccountMap::ActiveMap::const_iterator acc_it = config->accounts.active().begin();
        acc_it != config->accounts.active().end();
        ++acc_it, ++res_acc_i)
      {
        auto& res_acc = result->accounts[res_acc_i];
        res_acc.account_id = acc_it->first;
        res_acc.time_offset = GrpcAlgs::pack_time(acc_it->second->time_offset);
        res_acc.active = acc_it->second->get_status() == 'A';
        res_acc.budget = GrpcAlgs::pack_decimal(
          acc_it->second->budget + acc_it->second->paid_amount);
      }

      result->campaigns.resize(config->campaigns.active().size());
      result->ccgs.resize(config->campaigns.active().size());
      std::uint32_t res_campaign_i = 0;
      std::uint32_t res_ccg_i = 0;
      std::set<unsigned long> filled_campaign_ids;

      for (CampaignMap::ActiveMap::const_iterator cmp_it = config->campaigns.active().begin();
        cmp_it != config->campaigns.active().end();
        ++cmp_it, ++res_ccg_i)
      {
        auto& res_ccg = result->ccgs[res_ccg_i];

        AccountMap::ActiveMap::const_iterator acc_it =
          config->accounts.active().find(cmp_it->second->account_id);
        const Generics::Time time_offset = acc_it != config->accounts.active().end() ?
          acc_it->second->time_offset :
          Generics::Time::ZERO;

        const unsigned long campaign_id = cmp_it->second->campaign_group_id;

        res_ccg.ccg_id = cmp_it->first;
        res_ccg.campaign_id = campaign_id;
        res_ccg.active = cmp_it->second->get_status() == 'A';
        res_ccg.time_offset = GrpcAlgs::pack_time(time_offset);
        pack_delivery_limits(res_ccg.delivery_limits, cmp_it->second->ccg_delivery_limits);
        res_ccg.imp_amount = GrpcAlgs::pack_decimal(cmp_it->second->imp_revenue);
        res_ccg.click_amount = GrpcAlgs::pack_decimal(cmp_it->second->click_revenue);

        if (filled_campaign_ids.find(campaign_id) == filled_campaign_ids.end())
        {
          auto& res_campaign = result->campaigns[res_campaign_i++];
          res_campaign.campaign_id = campaign_id;
          res_campaign.active = true; // no separate status for campaign
          res_campaign.time_offset = GrpcAlgs::pack_time(time_offset);
          pack_delivery_limits(res_campaign.delivery_limits,
            cmp_it->second->campaign_delivery_limits);
        }
      }

      result->campaigns.resize(res_campaign_i);
      return result.release();
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }

    return nullptr; // never reach
  }

  AdServer::CampaignSvcs::BillStatInfo*
  CampaignServerBaseImpl::get_bill_stat()
    /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    BillStatSource::CStat_var bill_stat = bill_stat_.get();

    if (!bill_stat)
    {
      throw CampaignServer::NotReady("Billing statistics aren't loaded");
    }

    std::unique_ptr<BillStatInfo> result_bill_stat(new BillStatInfo());

    {
      result_bill_stat->accounts.resize(bill_stat->accounts.size());

      std::uint32_t acc_i = 0;
      for (BillStatSource::Stat::AccountMap::const_iterator acc_it = bill_stat->accounts.begin();
          acc_it != bill_stat->accounts.end(); ++acc_it, ++acc_i)
      {
        auto& acc_info = result_bill_stat->accounts[acc_i];
        acc_info.account_id = acc_it->first;
        fill_amount_distribution_(acc_info.amount_distribution, acc_it->second);
      }
    }

    {
      result_bill_stat->campaigns.resize(bill_stat->campaigns.size());

      std::uint32_t cmp_i = 0;
      for (BillStatSource::Stat::CampaignMap::const_iterator cmp_it = bill_stat->campaigns.begin();
          cmp_it != bill_stat->campaigns.end(); ++cmp_it, ++cmp_i)
      {
        auto& cmp_info = result_bill_stat->campaigns[cmp_i];
        cmp_info.campaign_id = cmp_it->first;
        fill_amount_count_distribution_(cmp_info.amount_count_distribution, cmp_it->second);
      }
    }

    {
      result_bill_stat->ccgs.resize(bill_stat->ccgs.size());

      std::uint32_t ccg_i = 0;
      for (BillStatSource::Stat::CCGMap::const_iterator ccg_it = bill_stat->ccgs.begin();
          ccg_it != bill_stat->ccgs.end(); ++ccg_it, ++ccg_i)
      {
        auto& ccg_info = result_bill_stat->ccgs[ccg_i];
        ccg_info.ccg_id = ccg_it->first;
        fill_amount_count_distribution_(ccg_info.amount_count_distribution, ccg_it->second);
      }
    }

    return result_bill_stat.release();
  }

  AdServer::CampaignSvcs::DetectorsConfig*
  CampaignServerBaseImpl::detectors(const AdServer::CampaignSvcs::TimestampInfo& request_timestamp)
    /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerBaseImpl::detectors()";

    try
    {
      CampaignConfig_var config = campaign_config();
      if (config.in() == 0)
      {
        throw CampaignServer::NotReady("Campaign configuration isn't loaded");
      }

      auto ret = std::make_unique<AdServer::CampaignSvcs::DetectorsConfig>();

      if (config->detectors_timestamp > GrpcAlgs::unpack_time(request_timestamp))
      {
        CampaignGetConfigSettings settings;
        settings.portion = 0;
        settings.portions_number = 1;
        settings.no_deleted = true;

        fill_search_enginies_(Generics::Time::ZERO, config, settings, ret->engines);

        fill_object_update_sequences_(
          ret->web_browsers,
          static_cast<AdServer::CampaignSvcs::DeletedStringIdSeq*>(0),
          config->web_browsers,
          WebBrowserFillAdapter(Generics::Time::ZERO, 0, 1),
          "web_browsers");

        fill_object_update_sequences_(
          ret->platforms,
          static_cast<AdServer::CampaignSvcs::DeletedIdSeq*>(0),
          config->platforms,
          PlatformFillAdapter(Generics::Time::ZERO, 0, 1),
          "platforms");
      }

      ret->timestamp = GrpcAlgs::pack_time(config->detectors_timestamp);

      return ret.release();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
    return 0; // never reach
  }

  /**
   *  CampaignServerImpl
   */
  CampaignServerImpl::CampaignServerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    ProcStatImpl* proc_stat_impl,
    unsigned long colo_id,
    const char* version,
    unsigned int config_update_period,
    unsigned int ecpm_update_period,
    const Generics::Time& bill_stat_update_period,
    const LogFlushTraits& colo_update_flush_traits,
    unsigned long server_id,
    const String::SubString& campaign_statuses,
    const String::SubString& channel_statuses,
    const char* pg_connection_string,
    const Generics::Time& stat_stamp_sync_period,
    const std::vector<std::string>& stat_providers,
    const Generics::Time& audience_expiration_time,
    const Generics::Time& pending_expire_time,
    bool enable_delivery_thresholds)
    /*throw(
      CampaignServerImpl::InvalidArgument,
      CampaignServerImpl::Exception,
      eh::Exception)*/
    : CampaignServerBaseImpl(
        callback,
        logger,
        proc_stat_impl,
        config_update_period,
        ecpm_update_period,
        bill_stat_update_period,
        colo_update_flush_traits,
        colo_id,
        version)
  {
    static const char* FUN = "CampaignServerImpl::CampaignServerImpl()";

    try
    {
      pg_env_ = new Commons::Postgres::Environment(pg_connection_string);

      add_child_object(pg_env_);

      campaign_config_db_source_ = new CampaignConfigDBSource(
        logger,
        server_id,
        campaign_statuses,
        channel_statuses,
        pg_env_,
        stat_stamp_sync_period,
        stat_providers,
        audience_expiration_time,
        pending_expire_time,
        enable_delivery_thresholds);

      bill_stat_db_source_ = new BillStatDBSource(logger_, pg_env_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  CampaignServerImpl::~CampaignServerImpl() noexcept
  {
  }

  CampaignConfigSource_var
  CampaignServerImpl::get_campaign_config_source() noexcept
  {
    return campaign_config_db_source_;
  }

  BillStatSource_var
  CampaignServerImpl::get_bill_stat_source() noexcept
  {
    return bill_stat_db_source_;
  }

  void
  CampaignServerImpl::change_db_state(bool activate)
    /*throw(Exception)*/
  {
    try
    {
      if (activate)
      {
        pg_env_->activate_object();
      }
      else
      {
        pg_env_->deactivate_object();
        pg_env_->wait_object();
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Cannot change state: Caught eh::Exception" << ex.what();
      throw Exception(ostr.str());
    }
  }

  bool
  CampaignServerImpl::get_db_state() noexcept
  {
    return pg_env_->active();
  }

  void
  CampaignServerImpl::update_stat()
    /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
      AdServer::CampaignSvcs::CampaignServer::ImplementationException)*/
  {
    static const char* FUN = "CampaignServerImpl::update_stat()";

    try
    {
      campaign_config_db_source_->update_stat();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw CampaignSvcs::CampaignServer::ImplementationException(ostr.str().str());
    }
  }

  AdServer::CampaignSvcs::StatInfo*
  CampaignServerImpl::get_stat()
    /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    try
    {
      StatSource::CStat_var stat = campaign_config_db_source_->stat();
      CampaignConfigModifier::CState_var state = campaign_config_db_source_->modify_state();

      auto result = std::make_unique<AdServer::CampaignSvcs::StatInfo>();

      result->campaigns.resize(stat->campaign_stats.size());

      std::uint32_t cmp_stat_i = 0;
      for (StatSource::Stat::CampaignStatMap::const_iterator cmp_stat_it =
            stat->campaign_stats.begin();
          cmp_stat_it != stat->campaign_stats.end();
          ++cmp_stat_it, ++cmp_stat_i)
      {
        AdServer::CampaignSvcs::CampaignStatInfo& campaign_stat = result->campaigns[cmp_stat_i];

        campaign_stat.campaign_id = cmp_stat_it->first;
        campaign_stat.amount = GrpcAlgs::pack_decimal(cmp_stat_it->second.amount);
        campaign_stat.comm_amount = GrpcAlgs::pack_decimal(cmp_stat_it->second.comm_amount);
        campaign_stat.daily_amount = GrpcAlgs::pack_decimal(cmp_stat_it->second.daily_amount);
        campaign_stat.daily_comm_amount = GrpcAlgs::pack_decimal(
          cmp_stat_it->second.daily_comm_amount);

        campaign_stat.ccgs.resize(cmp_stat_it->second.ccgs.size());
        std::uint32_t ccg_stat_i = 0;
        for (StatSource::Stat::CCGStatMap::const_iterator ccg_stat_it =
              cmp_stat_it->second.ccgs.begin();
            ccg_stat_it != cmp_stat_it->second.ccgs.end();
            ++ccg_stat_it, ++ccg_stat_i)
        {
          AdServer::CampaignSvcs::CCGStatInfo& ccg_stat = campaign_stat.ccgs[ccg_stat_i];
          ccg_stat.ccg_id = ccg_stat_it->first;
          ccg_stat.impressions = GrpcAlgs::pack_decimal(ccg_stat_it->second.impressions);
          ccg_stat.clicks = GrpcAlgs::pack_decimal(ccg_stat_it->second.clicks);
          ccg_stat.actions = GrpcAlgs::pack_decimal(ccg_stat_it->second.actions);
          ccg_stat.amount = GrpcAlgs::pack_decimal(ccg_stat_it->second.amount);
          ccg_stat.comm_amount = GrpcAlgs::pack_decimal(ccg_stat_it->second.comm_amount);
          ccg_stat.daily_amount = GrpcAlgs::pack_decimal(ccg_stat_it->second.daily_amount);
          ccg_stat.daily_comm_amount = GrpcAlgs::pack_decimal(
            ccg_stat_it->second.daily_comm_amount);

          ccg_stat.prev_hour_amount = GrpcAlgs::pack_decimal(ccg_stat_it->second.prev_hour_amount);
          ccg_stat.prev_hour_comm_amount = GrpcAlgs::pack_decimal(
            ccg_stat_it->second.prev_hour_comm_amount);
          ccg_stat.current_hour_amount = GrpcAlgs::pack_decimal(
            ccg_stat_it->second.cur_hour_amount);
          ccg_stat.current_hour_comm_amount = GrpcAlgs::pack_decimal(
            ccg_stat_it->second.cur_hour_comm_amount);

          ccg_stat.creatives.resize(ccg_stat_it->second.creatives.size());
          std::uint32_t cc_i = 0;
          for (StatSource::Stat::CCGStat::
                CreativeStatMap::const_iterator cc_it = ccg_stat_it->second.creatives.begin();
              cc_it != ccg_stat_it->second.creatives.end(); ++cc_it, ++cc_i)
          {
            AdServer::CampaignSvcs::CreativeStatInfo& cc_stat = ccg_stat.creatives[cc_i];
            cc_stat.cc_id = cc_it->first;
            cc_stat.impressions = GrpcAlgs::pack_decimal(cc_it->second.impressions);
            cc_stat.clicks = GrpcAlgs::pack_decimal(cc_it->second.clicks);
            cc_stat.actions = GrpcAlgs::pack_decimal(cc_it->second.actions);
          }

          ccg_stat.publishers.resize(ccg_stat_it->second.publisher_amounts.size());
          std::uint32_t pub_i = 0;
          for (StatSource::Stat::CCGStat::
                PublisherStatMap::const_iterator pub_it =
                  ccg_stat_it->second.publisher_amounts.begin();
              pub_it != ccg_stat_it->second.publisher_amounts.end();
              ++pub_it, ++pub_i)
          {
            AdServer::CampaignSvcs::PublisherCCGStatInfo& pub_stat = ccg_stat.publishers[pub_i];
            pub_stat.pub_account_id = pub_it->first;
            pub_stat.amount = GrpcAlgs::pack_decimal(pub_it->second.amount);
          }

          ccg_stat.tags.resize(ccg_stat_it->second.tag_stats.size());
          std::uint32_t tag_i = 0;
          for (StatSource::Stat::CCGStat::
                TagStatMap::const_iterator tag_it = ccg_stat_it->second.tag_stats.begin();
              tag_it != ccg_stat_it->second.tag_stats.end();
              ++tag_it, ++tag_i)
          {
            AdServer::CampaignSvcs::TagCCGStatInfo& tag_stat = ccg_stat.tags[tag_i];
            tag_stat.tag_id = tag_it->first;
            tag_stat.cur_pub_isp_amount = GrpcAlgs::pack_decimal(
              tag_it->second.current_hour_stat.isp_pub_amount);
            tag_stat.cur_adv_amount = GrpcAlgs::pack_decimal(
              tag_it->second.current_hour_stat.adv_amount);
            tag_stat.cur_adv_comm_amount = GrpcAlgs::pack_decimal(
              tag_it->second.current_hour_stat.adv_comm_amount);
            tag_stat.prev_pub_isp_amount = GrpcAlgs::pack_decimal(
              tag_it->second.prev_hour_stat.isp_pub_amount);
            tag_stat.prev_adv_amount = GrpcAlgs::pack_decimal(
              tag_it->second.prev_hour_stat.adv_amount);
            tag_stat.prev_adv_comm_amount = GrpcAlgs::pack_decimal(
              tag_it->second.prev_hour_stat.adv_comm_amount);
          }

          ccg_stat.ctr_resets.resize(ccg_stat_it->second.ctr_reset_stats.size());
          std::uint32_t ctr_reset_i = 0;
          for (StatSource::Stat::CCGStat::
                CtrResetStatMap::const_iterator ctr_reset_it =
                ccg_stat_it->second.ctr_reset_stats.begin();
              ctr_reset_it != ccg_stat_it->second.ctr_reset_stats.end();
              ++ctr_reset_it, ++ctr_reset_i)
          {
            AdServer::CampaignSvcs::CtrResetStatInfo& ctr_reset_stat =
              ccg_stat.ctr_resets[ctr_reset_i];
            ctr_reset_stat.ctr_reset_id = ctr_reset_it->first;
            ctr_reset_stat.impressions = GrpcAlgs::pack_decimal(ctr_reset_it->second.impressions);
          }
        }
      }

      result->accounts.resize(stat->account_amounts.size());
      std::uint32_t acc_stat_i = 0;
      for (StatSource::Stat::AccountAmountMap::const_iterator stat_it =
            stat->account_amounts.begin();
          stat_it != stat->account_amounts.end();
          ++stat_it, ++acc_stat_i)
      {
        result->accounts[acc_stat_i].id = stat_it->first;
        result->accounts[acc_stat_i].amount = GrpcAlgs::pack_decimal(stat_it->second.amount);
        result->accounts[acc_stat_i].comm_amount = GrpcAlgs::pack_decimal(
          stat_it->second.comm_amount);
        result->accounts[acc_stat_i].daily_amount = GrpcAlgs::pack_decimal(RevenueDecimal::ZERO);
        result->accounts[acc_stat_i].daily_comm_amount = GrpcAlgs::pack_decimal(
          RevenueDecimal::ZERO);
      }

      /*
      result->avg_rates.resize(state->ctr_ar.size());
      std::uint32_t rate_i = 0;
      for (CampaignConfigModifier::State::CtrArMap::const_iterator rate_it =
            state->ctr_ar.begin();
          rate_it != state->ctr_ar.end();
          ++rate_it, ++rate_i)
      {
        AdServer::CampaignSvcs::AvgRateInfo& avg_rate = result->avg_rates[rate_i];
        avg_rate.rate_type = rate_it->first.rate_type;
        avg_rate.country = rate_it->first.country_code;
        avg_rate.rate_sum = GrpcAlgs::pack_decimal(rate_it->second.rate_sum);
        avg_rate.divider = rate_it->second.divider;
      }
      */

      return result.release();
    }
    catch(const CampaignConfigDBSource::NotReady&)
    {
      throw CampaignServer::NotReady("Campaign statistics aren't ready");
    }
  }

  /**
   * CampaignServerProxyImpl
   */
  CampaignServerProxyImpl::CampaignServerProxyImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    ProcStatImpl* proc_stat_impl,
    unsigned long colo_id,
    const char* version,
    unsigned int config_update_period,
    unsigned int ecpm_update_period,
    const Generics::Time& bill_stat_update_period,
    const LogFlushTraits& colo_update_flush_traits,
    unsigned long server_id,
    const String::SubString& channel_statuses,
    const std::vector<std::string>& campaign_server_refs,
    const char* campaign_statuses,
    const char* countries,
    bool only_tags)
    /*throw(InvalidArgument, Exception, eh::Exception)*/
    : CampaignServerBaseImpl(
        callback,
        logger,
        proc_stat_impl,
        config_update_period,
        ecpm_update_period,
        bill_stat_update_period,
        colo_update_flush_traits,
        colo_id,
        version)
  {
    static const char* FUN = "CampaignServerProxyImpl::CampaignServerProxyImpl()";

    try
    {
      campaign_config_server_source_ = new CampaignConfigServerSource(
        logger,
        server_id,
        campaign_server_refs,
        colo_id,
        version,
        campaign_statuses,
        channel_statuses,
        countries,
        only_tags);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't init CampaignConfigServerSource: " << e.what();
      throw Exception(ostr);
    }

    try
    {
      bill_stat_server_source_ = new BillStatServerSource(logger_, server_id, campaign_server_refs);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't init BillStatServerSource: " << e.what();
      throw Exception(ostr);
    }
  }

  CampaignServerProxyImpl::~CampaignServerProxyImpl() noexcept
  {}

  CampaignConfigSource_var
  CampaignServerProxyImpl::get_campaign_config_source() noexcept
  {
    return campaign_config_server_source_;
  }

  BillStatSource_var
  CampaignServerProxyImpl::get_bill_stat_source() noexcept
  {
    return bill_stat_server_source_;
  }

  void CampaignServerProxyImpl::change_db_state(bool /*new_state*/)
    /*throw(Exception)*/
  {
    throw Exception("Not supported");
  }

  AdServer::CampaignSvcs::StatInfo*
  CampaignServerProxyImpl::get_stat()
    /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
      AdServer::CampaignSvcs::CampaignServer::NotReady)*/
  {
    throw AdServer::CampaignSvcs::CampaignServer::NotSupport("Not supported");
  }

  void
  CampaignServerProxyImpl::update_stat()
    /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
      AdServer::CampaignSvcs::CampaignServer::ImplementationException)*/
  {
    throw AdServer::CampaignSvcs::CampaignServer::NotSupport("Not supported");
  }
} // namespace AdServer::CampaignSvcs
