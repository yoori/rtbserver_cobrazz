/**
 * @file RequestLogLoader.cpp
 */

#include "RequestInfoManagerStats.hpp"
#include "RequestLogLoader.hpp"
#include <Generics/DirSelector.hpp>

#include <Commons/DelegateActiveObject.hpp>
#include <LogCommons/Request.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <LogCommons/TagRequest.hpp>
#include "CompositeMetricsProviderRIM.hpp"
/*
 * LogFetcherBase - base class for process one type logs
 * LogRecordFetcher - LogFetcherBase implementation
 * CheckLogsTask - periodical task for check one type input logs files
 * LogProcessingRunner - active object that control logs processing threads
 */
namespace
{
  const char DEFAULT_ERROR_DIR[] = "Error";
  const unsigned long FETCH_FILES_LIMIT = 50000;

  void
  fill_revenue_block(
    const AdServer::LogProcessing::RequestData::Revenue& rev,
    AdServer::RequestInfoSvcs::RequestInfo::Revenue& res_rev)
    noexcept
  {
    res_rev.rate_id = rev.rate_id;
    res_rev.impression = rev.imp_revenue;
    res_rev.click = rev.click_revenue;
    res_rev.action = rev.action_revenue;
  }
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  // LogRecordFetcherBase
  class LogRecordFetcherBase: public LogFetcherBase
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    LogRecordFetcherBase(
      Generics::ActiveObjectCallback* callback,
      LogProcessingState* processing_state,
      const char* folder,
      unsigned int priority,
      const Generics::Time& check_period,
      RequestInfoManagerStatsImpl* proc_stat_impl,
      CompositeMetricsProviderRIM* cmprim)
      noexcept;

    virtual
    Generics::Time
    check_files() noexcept;

    virtual void
    process(LogProcessing::FileReceiver::FileGuard* file_ptr) noexcept;

  protected:
    virtual
    ~LogRecordFetcherBase() noexcept
    {}

    // return true if file processed fully
    virtual bool
    process_file_(
      AdServer::LogProcessing::LogFileNameInfo& name_info,
      LogProcessing::FileReceiver::FileGuard* file)
      /*throw(eh::Exception)*/ = 0;

    virtual const char*
    log_base_name_() noexcept = 0;

    virtual unsigned long
    log_index_() noexcept = 0;

  protected:
    void
    file_move_back_to_input_dir_(
      const AdServer::LogProcessing::LogFileNameInfo& info,
      const char* file_name) /*throw(eh::Exception)*/;

  protected:
    Generics::ActiveObjectCallback_var log_errors_;
    LogProcessingState_var processing_state_;

    // check configuration
    const std::string DIR_;
    const Generics::Time check_period_;

    RequestInfoManagerStatsImpl_var proc_stat_impl_;
  };

  // LogRecordFetcher template class
  template <typename LogTraitsType, typename ProcessFun>
  class LogRecordFetcher: public LogRecordFetcherBase
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    LogRecordFetcher(
      Generics::ActiveObjectCallback* callback,
      LogProcessingState* processing_state,
      const char* folder,
      unsigned int priority,
      const Generics::Time& check_period,
      const ProcessFun& process_fun,
      RequestInfoManagerStatsImpl* proc_stat_impl,
      CompositeMetricsProviderRIM* cmprim)
      noexcept;

  protected:
    virtual
    ~LogRecordFetcher() noexcept
    {}

    virtual bool
    process_file_(
      AdServer::LogProcessing::LogFileNameInfo& name_info,
      LogProcessing::FileReceiver::FileGuard* file)
      /*throw(eh::Exception)*/;

    virtual const char*
    log_base_name_() noexcept;

    virtual unsigned long
    log_index_() noexcept;

  private:
    const ProcessFun process_fun_;
  };

  // LogRecordFetcher template class
  class RequestOperationFetcher: public LogRecordFetcherBase
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    RequestOperationFetcher(
      Generics::ActiveObjectCallback* callback,
      LogProcessingState* processing_state,
      const char* folder,
      unsigned int priority,
      const Generics::Time& check_period,
      RequestOperationProcessor* request_operation_processor,
      RequestInfoManagerStatsImpl* proc_stat_impl,
      CompositeMetricsProviderRIM* cmprim)
      noexcept;

  protected:
    virtual
    ~RequestOperationFetcher() noexcept
    {}

    virtual bool
    process_file_(
      AdServer::LogProcessing::LogFileNameInfo& name_info,
      LogProcessing::FileReceiver::FileGuard* file)
      /*throw(eh::Exception)*/;

    virtual const char*
    log_base_name_() noexcept;

    virtual unsigned long
    log_index_() noexcept;

  protected:
    RequestOperationLoader_var request_operation_loader_;
  };

  /* CheckLogsTask */
  class CheckLogsTask: public Generics::TaskGoal
  {
  public:
    CheckLogsTask(
      Generics::Planner* planner,
      Generics::TaskRunner* task_runner,
      LogFetcherBase* log_fetcher)
      noexcept
      : Generics::TaskGoal(task_runner),
        planner_(planner),
        log_fetcher_(ReferenceCounting::add_ref(log_fetcher))
    {}

    virtual void
    execute() noexcept
    {
      Generics::Time next_check = log_fetcher_->check_files();
      planner_->schedule(this, next_check);
    }

  private:
    Generics::Planner* planner_;
    LogFetcher_var log_fetcher_;
  };

  /** LogFetcherBase */
  LogFetcherBase::LogFetcherBase(
    unsigned int priority,
    LogProcessing::FileReceiver* file_receiver,
    CompositeMetricsProviderRIM* cmprim) noexcept
    : priority_(priority), 
      file_receiver_(ReferenceCounting::add_ref(file_receiver)), 
      cmprim_(ReferenceCounting::add_ref(cmprim))
  {}

  unsigned int
  LogFetcherBase::priority() const noexcept
  {
    return priority_;
  }

  LogProcessing::FileReceiver_var
  LogFetcherBase::file_receiver() noexcept
  {
    return file_receiver_;
  }

  // LogRecordFetcherBase implementation
  LogRecordFetcherBase::LogRecordFetcherBase(
    Generics::ActiveObjectCallback* callback,
    LogProcessingState* processing_state,
    const char* folder,
    unsigned int priority,
    const Generics::Time& check_period,
    RequestInfoManagerStatsImpl* proc_stat_impl,
    CompositeMetricsProviderRIM* cmprim)
    noexcept
    : LogFetcherBase(
        priority,
        LogProcessing::FileReceiver_var(
          new LogProcessing::FileReceiver(
            (std::string(folder) + "/Intermediate").c_str(),
            FETCH_FILES_LIMIT,
            processing_state->interrupter,
            nullptr)),
        cmprim),
      log_errors_(ReferenceCounting::add_ref(callback)),
      processing_state_(ReferenceCounting::add_ref(processing_state)),
      DIR_(folder),
      check_period_(check_period),
      proc_stat_impl_(ReferenceCounting::add_ref(proc_stat_impl))
  {}

  void
  LogRecordFetcherBase::process(LogProcessing::FileReceiver::FileGuard* file_ptr)
    noexcept
  {
    static const char* FUN = "LogRecordFetcherBase::process()";

    // file guard must be destroyed after moving file into errors store
    LogProcessing::FileReceiver::FileGuard_var file =
      ReferenceCounting::add_ref(file_ptr);
    AdServer::LogProcessing::LogFileNameInfo name_info;

    try
    {
      if (!file)
      {
        return;
      }

      name_info = AdServer::LogProcessing::LogFileNameInfo();
      parse_log_file_name(file->file_name().c_str(), name_info);

      if(!process_file_(name_info, file))
      {
        file_move_back_to_input_dir_(name_info, file->full_path().c_str());
      }
      else
      {
        if(::unlink(file->full_path().c_str()) != 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't delete file '" << file->full_path() << "'";
          log_errors_->report_error(
            Generics::ActiveObjectCallback::ERROR, ostr.str());
        }
            std::map<std::string, std::string> m;
            m["type"] = name_info.base_name;
            cmprim_->add_value_prometheus("processedFilesByType",m,1);
            cmprim_->add_value_prometheus("processedRecordCountByType",m,name_info.processed_lines_count);
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;

      // copy the erroneous file to the error folder
      if (file)
      {
        try
        {
          AdServer::LogProcessing::FileStore file_store(
            DIR_, DEFAULT_ERROR_DIR);
          file_store.store(file->full_path(), name_info.processed_lines_count);
        }
        catch (const eh::Exception& store_ex)
        {
          ostr << FUN << store_ex.what() << " Can't copy the file '" <<
            file->full_path() << "' to the error folder. Initial error: " <<
            ex.what();
        }

        ostr << " exception on file '" << file->file_name() << "':" << ex.what();
      }
      else
      {
        ostr << FUN << ": exception on getting eldest file :" << ex.what();
      }
      log_errors_->report_error(
        Generics::ActiveObjectCallback::ERROR, ostr.str());
    }
  }

  Generics::Time
  LogRecordFetcherBase::check_files() noexcept
  {
    static const char* FUN = "LogRecordFetcher::check_files()";

    try
    {
      Generics::Timer files_search_timer;

      if (proc_stat_impl_)
      {
        files_search_timer.start();
      }

      file_receiver_->fetch_files(DIR_.c_str(), log_base_name_());

      if (proc_stat_impl_)
      {
        files_search_timer.stop();
        proc_stat_impl_->add_load_time(
          log_index_(),
          files_search_timer.elapsed_time());
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      log_errors_->report_error(
        Generics::ActiveObjectCallback::ERROR, ostr.str());
    }

    return Generics::Time::get_time_of_day() + check_period_;
  }

  void
  LogRecordFetcherBase::file_move_back_to_input_dir_(
    const AdServer::LogProcessing::LogFileNameInfo& info,
    const char* file_name) /*throw(eh::Exception)*/
  {
    static const char* FUN = "LogRecordFetcher::file_move_back_to_input_dir_()";

    std::string path;
    const char* ptr = strrchr(file_name, '/');

    if (ptr)
    {
      path.assign(file_name, ptr + 1);
    }

    const std::string new_file_name =
      AdServer::LogProcessing::restore_log_file_name(info, path);

    if (new_file_name != file_name)
    {
      if (::rename(file_name, new_file_name.c_str()))
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't move '" << file_name <<
          "' to '" << new_file_name << "'";

        log_errors_->report_error(
          Generics::ActiveObjectCallback::ERROR,
          ostr.str());
      }
    }
  }

  // LogRecordFetcher implementation
  template <typename LogTraitsType, typename ProcessFun>
  LogRecordFetcher<LogTraitsType, ProcessFun>::LogRecordFetcher(
    Generics::ActiveObjectCallback* callback,
    LogProcessingState* processing_state,
    const char* folder,
    unsigned int priority,
    const Generics::Time& check_period,
    const ProcessFun& process_fun,
    RequestInfoManagerStatsImpl* proc_stat_impl,
    CompositeMetricsProviderRIM* cmprim)
    noexcept
    : LogRecordFetcherBase(
        callback,
        processing_state,
        folder,
        priority,
        check_period,
        proc_stat_impl,
        cmprim),
      process_fun_(process_fun)
  {
    if (proc_stat_impl_)
    {
      LogStats stats;
      proc_stat_impl_->fill_values(Type2Index<LogTraitsType>::result, stats);
    }
 }

  template <typename LogTraitsType, typename ProcessFun>
  bool
  LogRecordFetcher<LogTraitsType, ProcessFun>::process_file_(
    AdServer::LogProcessing::LogFileNameInfo& name_info,
    LogProcessing::FileReceiver::FileGuard* file)
    /*throw(eh::Exception)*/
  {
    //static const char* FUN = "LogRecordFetcher::process_file_()";

    LogStats stats;

    typename LogTraitsType::CollectorType collector;
    std::ifstream ifs(file->full_path().c_str());
    typename LogTraitsType::IoHelperType(collector).load(ifs);

    Generics::Timer process_timer;

    if(proc_stat_impl_)
    {
      process_timer.start();
    }

    typename LogTraitsType::CollectorType::const_iterator it = collector.begin();
    std::advance(it, name_info.processed_lines_count);
    bool terminated = false;

    // process records
    for(; it != collector.end();
      ++it, ++stats.processed_records, ++name_info.processed_lines_count)
    {
      if (!processing_state_->interrupter->active())
      {
        terminated = true;
        break;
      }

      process_fun_(*it);
    }

    // Dump stats
    if(proc_stat_impl_)
    {
      if(!terminated)
      {
        ++stats.file_counter;
        process_timer.stop();
        stats.last_processed_file_timestamp = name_info.timestamp;
        stats.process_time += process_timer.elapsed_time();
      }
      proc_stat_impl_->fill_values(Type2Index<LogTraitsType>::result, stats);
    }

    return !terminated;
  }

  template <typename LogTraitsType, typename ProcessFun>
  const char*
  LogRecordFetcher<LogTraitsType, ProcessFun>::log_base_name_()
    noexcept
  {
    return LogTraitsType::log_base_name();
  }

  template <typename LogTraitsType, typename ProcessFun>
  unsigned long
  LogRecordFetcher<LogTraitsType, ProcessFun>::log_index_()
    noexcept
  {
    return Type2Index<LogTraitsType>::result;
  }

  // RequestOperationFetcher implementation
  RequestOperationFetcher::RequestOperationFetcher(Generics::ActiveObjectCallback* callback,
    LogProcessingState* processing_state,
    const char* folder,
    unsigned int priority,
    const Generics::Time& check_period,
    RequestOperationProcessor* request_operation_processor,
    RequestInfoManagerStatsImpl* proc_stat_impl,
    CompositeMetricsProviderRIM *cmprim)
    noexcept
    : LogRecordFetcherBase(
        callback,
        processing_state,
        folder,
        priority,
        check_period,
        proc_stat_impl,
        cmprim),
      request_operation_loader_(
        new RequestOperationLoader(request_operation_processor))
  {}

  bool
  RequestOperationFetcher::process_file_(
    AdServer::LogProcessing::LogFileNameInfo& name_info,
    LogProcessing::FileReceiver::FileGuard* file)
    /*throw(eh::Exception)*/
  {
    return request_operation_loader_->process_file(
      name_info.processed_lines_count,
      file->full_path().c_str(),
      processing_state_->interrupter);
  }

  const char*
  RequestOperationFetcher::log_base_name_()
    noexcept
  {
    return "RequestOperation";
  }

  unsigned long
  RequestOperationFetcher::log_index_()
    noexcept
  {
    return 0;
  }

  /*
   * Record processors (functors)
   */
  struct RequestRecordProcessor
  {
    RequestRecordProcessor(
      Generics::ActiveObjectCallback* callback,
      RequestContainerProcessor* request_container_processor)
      : callback_(ReferenceCounting::add_ref(callback)),
        request_processor_(
          ReferenceCounting::add_ref(request_container_processor))
    {}

    RequestRecordProcessor(const RequestRecordProcessor& r) noexcept
      : callback_(const_cast<Generics::ActiveObjectCallback*>(
          ReferenceCounting::add_ref(r.callback_))),
        request_processor_(ReferenceCounting::add_ref(r.request_processor_))
    {}

    void operator()(
      const AdServer::LogProcessing::
      RequestTraits::CollectorType::DataT& req) const
    {
      RequestInfo request_info(req.request_id());

      request_info.user_id = req.user_id();
      request_info.household_id = req.household_id();
      request_info.user_status = req.user_status();
      request_info.time = req.time();
      request_info.isp_time = req.isp_time();
      request_info.pub_time = req.pub_time();
      request_info.adv_time = req.adv_time();
      request_info.test_request = req.test_request();
      request_info.walled_garden = req.walled_garden();
      request_info.hid_profile = req.hid_profile();
      request_info.disable_fraud_detection = req.disable_fraud_detection();
      request_info.fraud = RequestInfo::RS_NORMAL;

      request_info.colo_id = req.colo_id();
      request_info.publisher_account_id = req.publisher_account_id();
      request_info.site_id = req.site_id();
      request_info.tag_id = req.tag_id();
      request_info.size_id = req.size_id().present() ? *req.size_id() : 0;
      request_info.ext_tag_id = req.ext_tag_id();
      request_info.adv_account_id = req.adv_account_id();
      request_info.advertiser_id = req.advertiser_id();
      request_info.campaign_id = req.cmp_id();
      request_info.ccg_id = req.ccg_id();
      request_info.ctr_reset_id = req.ctr_reset_id();
      request_info.cc_id = req.cc_id();
      request_info.has_custom_actions = req.has_custom_actions();
      request_info.tag_delivery_threshold = req.delivery_threshold();

      request_info.currency_exchange_id = req.currency_exchange_id();

      fill_revenue_block(req.adv_revenue(), request_info.adv_revenue);
      request_info.adv_revenue.currency_rate = req.adv_currency_rate();
      fill_revenue_block(req.adv_comm_revenue(), request_info.adv_comm_revenue);
      fill_revenue_block(req.adv_payable_comm_amount(), request_info.adv_payable_comm_amount);

      fill_revenue_block(req.pub_revenue(), request_info.pub_revenue);
      request_info.pub_revenue.currency_rate = req.pub_currency_rate();
      request_info.pub_bid_cost = RevenueDecimal::div(
        req.ecpm(), AdServer::CampaignSvcs::ECPM_FACTOR);
      request_info.pub_floor_cost = req.floor_cost();
      fill_revenue_block(req.pub_comm_revenue(), request_info.pub_comm_revenue);
      request_info.pub_commission = req.pub_commission();

      fill_revenue_block(req.isp_revenue(), request_info.isp_revenue);
      request_info.isp_revenue.currency_rate = req.isp_currency_rate();
      request_info.isp_revenue_share = req.isp_revenue_share();

      request_info.country = req.country_code();
      request_info.referer = req.referer();

      for(AdServer::LogProcessing::UserPropertyList::const_iterator
            it = req.user_props().begin();
          it != req.user_props().end();
          ++it)
      {
        if(it->first == "ClientVersion")
        {
          request_info.client_app_version = it->second;
        }
        else if(it->first == "Client")
        {
          request_info.client_app = it->second;
        }
        else if(it->first == "BrowserVersion")
        {
          request_info.browser_version = it->second;
        }
        else if(it->first == "OsVersion")
        {
          request_info.os_version = it->second;
        }
      }

      request_info.expression = req.channel_expression();
      request_info.channels.assign(
        req.channel_list().begin(),
        req.channel_list().end());
      request_info.geo_channels.assign(req.geo_channels().begin(),
        req.geo_channels().end());
      request_info.geo_channel_id = req.geo_channels().empty() ? 0 :
        req.geo_channels().front();
      request_info.device_channel_id = req.device_channel_id().present() ?
        req.device_channel_id().get() : 0;

      request_info.ccg_keyword_id = req.ccg_keyword_id();
      request_info.keyword_id = req.keyword_id();
      request_info.num_shown = req.num_shown();
      request_info.position = req.position();
      request_info.text_campaign = (req.ccg_type() == 'T');
      request_info.tag_size = req.tag_size();

      if(req.tag_visibility().present())
      {
        request_info.tag_visibility = req.tag_visibility().get();
      }

      if(req.tag_top_offset().present())
      {
        request_info.tag_top_offset = req.tag_top_offset().get();
      }

      if(req.tag_left_offset().present())
      {
        request_info.tag_left_offset = req.tag_left_offset().get();
      }

      for(LogProcessing::RequestData::CmpChannelList::const_iterator
            cmp_ch_it = req.cmp_channel_list().begin();
          cmp_ch_it != req.cmp_channel_list().end(); ++cmp_ch_it)
      {
        RequestInfo::ChannelRevenue ch_rev;
        ch_rev.channel_id = cmp_ch_it->channel_id;
        ch_rev.channel_rate_id = cmp_ch_it->channel_rate_id;
        ch_rev.impression = cmp_ch_it->imp_revenue;
        ch_rev.sys_impression = cmp_ch_it->imp_sys_revenue;
        ch_rev.adv_impression = cmp_ch_it->adv_imp_revenue;
        ch_rev.click = cmp_ch_it->click_revenue;
        ch_rev.sys_click = cmp_ch_it->click_sys_revenue;
        ch_rev.adv_click = cmp_ch_it->adv_click_revenue;
        request_info.cmp_channels.push_back(ch_rev);
      }

      request_info.enabled_notice = req.enabled_notice();
      request_info.disabled_pub_cost_check = req.disabled_pub_cost_check();
      request_info.enabled_impression_tracking = req.enabled_impression_tracking();
      request_info.enabled_action_tracking = req.enabled_action_tracking();

      request_info.auction_type = req.auction_type();

      //request_info.global_request_id = req.global_request_id();
      request_info.url = req.referer();
      request_info.ip_address = req.ip_address();
      request_info.user_channels.assign(
        req.history_channel_list().begin(),
        req.history_channel_list().end());
      request_info.ctr_algorithm_id = req.ctr_algorithm_id();
      request_info.referer_hash = req.full_referer_hash();
      request_info.viewability = -1; // on request viewability unknown
      request_info.ctr = req.ctr();
      request_info.campaign_freq = req.campaign_freq();
      request_info.conv_rate_algorithm_id = req.conv_rate_algorithm_id();
      request_info.conv_rate = req.conv_rate();
      //request_info.tag_predicted_viewability = req.tag_predicted_viewability();

      request_info.model_ctrs.assign(req.model_ctrs().begin(), req.model_ctrs().end());

      request_info.self_service_commission = req.self_service_commission();
      request_info.adv_commission = req.adv_commission();
      request_info.pub_cost_coef = req.pub_cost_coef();
      request_info.at_flags = req.at_flags();

      request_processor_->process_request(request_info);
    }

  private:
    mutable Generics::ActiveObjectCallback_var callback_;
    RequestContainerProcessor_var request_processor_;
  };

  struct ClickRecordProcessor
  {
    ClickRecordProcessor(
      UnmergedClickProcessor* unmerged_click_processor,
      RequestContainerProcessor* request_container_processor,
      RequestContainerProcessor::ActionType act)
      : click_processor_(
          ReferenceCounting::add_ref(unmerged_click_processor)),
        request_processor_(
          ReferenceCounting::add_ref(request_container_processor)),
        act_(act)
    {}

    void operator()(
      const AdServer::LogProcessing::ClickData& req)
      const
    {
      UnmergedClickProcessor::ClickInfo click_info;
      click_info.time = req.time().time();
      click_info.request_id = req.request_id();
      click_info.referer = req.referrer().get();

      click_processor_->process_click(click_info);

      request_processor_->process_action(act_, req.time().time(),
        req.request_id());
    }

  private:
    UnmergedClickProcessor_var click_processor_;
    RequestContainerProcessor_var request_processor_;
    RequestContainerProcessor::ActionType act_;
  };

  struct ImpressionRecordProcessor
  {
    ImpressionRecordProcessor(
      RequestContainerProcessor* request_container_processor)
      : request_processor_(
          ReferenceCounting::add_ref(request_container_processor))
    {}

    void operator()(
      const AdServer::LogProcessing::
      ImpressionTraits::CollectorType::DataT& req)
      const
    {
      if(req.request_type() == 'C')
      {
        request_processor_->process_impression_post_action(
          req.request_id(),
          RequestPostActionInfo(req.time().time(), req.action_name()));
      }
      else
      {
        ImpressionInfo impression_info;
        impression_info.request_id = req.request_id();
        impression_info.time = req.time().time();
        impression_info.verify_impression = (req.request_type() == 'T');
        impression_info.user_id = req.user_id();
        impression_info.viewability = req.viewability();

        // pub_sys_revenue not used
        if(req.pub_revenue().present() /*&& req.pub_sys_revenue().present()*/)
        {
          ImpressionInfo::PubRevenue pub_revenue;
          pub_revenue.revenue_type = (
            req.pub_revenue_type() == 'P' ?
            AdServer::CampaignSvcs::RT_SHARE :
            AdServer::CampaignSvcs::RT_ABSOLUTE);
          pub_revenue.impression = *req.pub_revenue();
          // pub_revenue.sys_impression = *req.pub_sys_revenue();
          impression_info.pub_revenue = pub_revenue;
        }
        request_processor_->process_impression(impression_info);
      }
    }

  private:
    RequestContainerProcessor_var request_processor_;
  };

  struct AdvertiserActionRecordProcessor
  {
    AdvertiserActionRecordProcessor(
      AdvActionProcessor* adv_action_processor)
      : adv_action_processor_(
          ReferenceCounting::add_ref(adv_action_processor))
    {}

    void operator()(
      const AdServer::LogProcessing::
      AdvertiserActionTraits::CollectorType::DataT& req)
      const
    {
      if(req.action_id().present())
      {
        AdvActionProcessor::AdvExActionInfo adv_ex_action_info;

        adv_ex_action_info.user_id = req.user_id();
        adv_ex_action_info.time = req.time().time();
        adv_ex_action_info.action_id = req.action_id().get();
        adv_ex_action_info.device_channel_id = req.device_channel_id().get();
        adv_ex_action_info.action_request_id =
          req.action_request_id().present() ? req.action_request_id().get() :
          AdServer::Commons::UserId();
        adv_ex_action_info.order_id = req.order_id().present() ?
          std::string(req.order_id().get()) : std::string();
        adv_ex_action_info.action_value = req.cur_value();
        adv_ex_action_info.referer =
          req.referrer().present() ? req.referrer().get() : "";
        adv_ex_action_info.ip_address =
          req.ip_address().present() ? req.ip_address().get() : "";

        std::copy(
          req.ccg_ids().begin(),
          req.ccg_ids().end(),
          std::back_inserter(adv_ex_action_info.ccg_ids));

        adv_action_processor_->process_custom_action(adv_ex_action_info);
      }
      else
      {
        AdvActionProcessor::AdvActionInfo adv_action_info;

        adv_action_info.user_id = req.user_id();
        adv_action_info.time = req.time().time();

        for(AdServer::LogProcessing::NumberList::const_iterator ccg_it =
              req.ccg_ids().begin();
            ccg_it != req.ccg_ids().end(); ++ccg_it)
        {
          adv_action_info.ccg_id = *ccg_it;
          adv_action_processor_->process_adv_action(adv_action_info);
        }
      }
    }

  private:
    AdvActionProcessor_var adv_action_processor_;
  };

  /** PassbackImpressionRecordProcessor */
  struct PassbackImpressionRecordProcessor
  {
    PassbackImpressionRecordProcessor(
      PassbackVerificationProcessor* passback_verification_processor)
      : passback_verification_processor_(
          ReferenceCounting::add_ref(passback_verification_processor))
    {}

    void operator()(
      const AdServer::LogProcessing::
      PassbackImpressionTraits::CollectorType::DataT& req)
      const
    {
      passback_verification_processor_->process_passback_request(
        req.request_id(), req.time().time());
    }

  private:
    PassbackVerificationProcessor_var passback_verification_processor_;
  };

  /** TagRequestRecordProcessor */
  struct TagRequestRecordProcessor
  {
    TagRequestRecordProcessor(
      TagRequestProcessor* tag_request_processor)
      : tag_request_processor_(
          ReferenceCounting::add_ref(tag_request_processor))
    {}

    void
    operator ()(
      const LogProcessing::TagRequestTraits::CollectorType::DataT& req) const
    {
      TagRequestInfo tag_request_info;
      tag_request_info.time = req.time().time();
      tag_request_info.isp_time = req.isp_time().time();
      tag_request_info.colo_id = req.colo_id();
      tag_request_info.tag_id = req.tag_id();
      tag_request_info.size_id = req.size_id().present() ? *req.size_id() : 0;
      tag_request_info.ext_tag_id = req.ext_tag_id();
      tag_request_info.referer = req.referer();
      tag_request_info.urls = req.urls();
      tag_request_info.referer_hash = req.full_referer_hash();

      tag_request_info.user_status = req.user_status();
      tag_request_info.country = req.country();
      tag_request_info.request_id = req.passback_request_id();
      tag_request_info.floor_cost = req.floor_cost();

      if(req.opt_in_section().present())
      {
        tag_request_info.user_id = req.opt_in_section().get().user_id();
        tag_request_info.ad_shown = req.opt_in_section().get().ad_shown();
        tag_request_info.user_agent = new Commons::StringHolder(
          req.opt_in_section().get().user_agent());
        if(!tag_request_info.user_id.is_null())
        {
          tag_request_info.site_id = req.opt_in_section().get().site_id();
          tag_request_info.page_load_id = req.opt_in_section().get().page_load_id();
          tag_request_info.profile_referer = req.opt_in_section().get().profile_referer();
        }
      }
      else
      {
        tag_request_info.site_id = 0;
        tag_request_info.ad_shown = false;
        tag_request_info.profile_referer = false;
      }

      tag_request_processor_->process_tag_request(tag_request_info);
    }

  private:
    TagRequestProcessor_var tag_request_processor_;
  };

  /**
   * Use to facilitate the creation of objects, autodetection of ProcessFun
   */
  template <typename Traits, typename ProcessFun>
  ReferenceCounting::SmartPtr<LogRecordFetcher<Traits, ProcessFun> >
  make_fetcher(
    CompositeMetricsProviderRIM* cmprim,
    Generics::ActiveObjectCallback* callback,
    LogProcessingState* processing_state,
    const InLog& in_log,
    const Generics::Time& check_period,
    const ProcessFun& process_fun,
    RequestInfoManagerStatsImpl* proc_stat_impl = 0)
  {
    return new LogRecordFetcher<Traits, ProcessFun>(
      callback,
      processing_state,
      in_log.dir.c_str(),
      in_log.priority,
      check_period,
      process_fun,
      proc_stat_impl,
      cmprim);
  }

  /** InLog */
  InLog::InLog() noexcept
    : priority()
  {}

  /** RequestLogLoader */
  RequestLogLoader::RequestLogLoader(Generics::ActiveObjectCallback* callback,
    const InLogs& in_logs,
    UnmergedClickProcessor* unmerged_click_processor,
    RequestContainerProcessor* request_container_processor,
    AdvActionProcessor* adv_action_processor,
    PassbackVerificationProcessor* passback_verification_processor,
    TagRequestProcessor* tag_request_processor,
    RequestOperationProcessor* request_operation_processor,
    const Generics::Time& check_period,
    const Generics::Time&,
    std::size_t threads_count,
    RequestInfoManagerStatsImpl* proc_stat_impl,
    CompositeMetricsProviderRIM *cmprim)
    /*throw(Exception)*/
    : log_errors_callback_(ReferenceCounting::add_ref(callback)),
      processing_state_(new LogProcessingState()),
      cmprim_(ReferenceCounting::add_ref(cmprim))
  {
    using namespace AdServer::LogProcessing;

    // initialize log files processing contexts
    log_fetchers_[RequestLogType] = make_fetcher<RequestTraits>(
      cmprim_,
      log_errors_callback_,
      processing_state_,
      in_logs.request,
      check_period,
      RequestRecordProcessor(
        log_errors_callback_,
        request_container_processor),
      proc_stat_impl);

    log_fetchers_[ImpressionLogType] = make_fetcher<ImpressionTraits>(
      cmprim_,
      log_errors_callback_,
      processing_state_,
      in_logs.impression,
      check_period,
      ImpressionRecordProcessor(request_container_processor),
      proc_stat_impl);

    log_fetchers_[ClickLogType] = make_fetcher<ClickTraits>(
      cmprim_,
      log_errors_callback_,
      processing_state_,
      in_logs.click,
      check_period,
      ClickRecordProcessor(
        unmerged_click_processor,
        request_container_processor,
        RequestContainerProcessor::AT_CLICK),
      proc_stat_impl);

    log_fetchers_[AdvertiserActionLogType] = make_fetcher<AdvertiserActionTraits>(
      cmprim_,
      log_errors_callback_,
      processing_state_,
      in_logs.advertiser_action,
      check_period,
      AdvertiserActionRecordProcessor(adv_action_processor),
      proc_stat_impl);

    log_fetchers_[PassbackImpressionLogType] = make_fetcher<PassbackImpressionTraits>(
      cmprim_,
      log_errors_callback_,
      processing_state_,
      in_logs.passback_impression,
      check_period,
      PassbackImpressionRecordProcessor(passback_verification_processor),
      proc_stat_impl);

    log_fetchers_[TagRequestLogType] = make_fetcher<TagRequestTraits>(
      cmprim_,
      log_errors_callback_,
      processing_state_,
      in_logs.tag_request,
      check_period,
      TagRequestRecordProcessor(tag_request_processor),
      proc_stat_impl);

    log_fetchers_[RequestOperationLogType] = LogFetcher_var(new RequestOperationFetcher(
      log_errors_callback_,
      processing_state_,
      in_logs.request_operation.dir.c_str(),
      in_logs.request_operation.priority,
      check_period,
      request_operation_processor,
      proc_stat_impl,
      cmprim_));

    add_child_object(processing_state_->interrupter);

    // install log files check tasks
    scheduler_ = new Generics::Planner(log_errors_callback_);

    add_child_object(scheduler_);

    log_fetch_runner_ = new Generics::TaskRunner(
      log_errors_callback_, log_fetchers_.size());

    add_child_object(log_fetch_runner_);
    FileReceiverFacade::FileReceiversInitializer file_receivers;
    std::vector<std::size_t> priorities(LogTypesCount);

    for(LogFetchers::const_iterator fetcher_it =
          log_fetchers_.begin();
        fetcher_it != log_fetchers_.end(); ++fetcher_it)
    {
      file_receivers.emplace_back(
        fetcher_it->first, fetcher_it->second->file_receiver());
      priorities[fetcher_it->first] = fetcher_it->second->priority();

      Generics::Task_var task(new CheckLogsTask(
        scheduler_, log_fetch_runner_, fetcher_it->second));
      log_fetch_runner_->enqueue_task(task);
    }

    // initialize processing threads
    file_receiver_facade_ = new FileReceiverFacade(
      file_receivers, OrderStrategy(priorities));
    add_child_object(file_receiver_facade_);

    Commons::DelegateActiveObject_var process_files_worker =
      Commons::make_delegate_active_object(
        std::bind(&RequestLogLoader::process_file_, this),
         callback,
         threads_count);
    add_child_object(process_files_worker);
  }

  void
  RequestLogLoader::process_file_() noexcept
  {
    static const char* FUN = "LogRecordFetcherBase::process()";

    try
    {
      FileReceiverFacade::FileEntity file =
        file_receiver_facade_->get_eldest();

      if (file.file_guard)
      {
        log_fetchers_[file.type]->process(file.file_guard);
      }
    }
    catch (FileReceiverFacade::Interrupted&)
    {}
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();
      log_errors_callback_->report_error(
        Generics::ActiveObjectCallback::ERROR, ostr.str());
    }
  }
}
}
