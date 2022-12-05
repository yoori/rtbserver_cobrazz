#include <Generics/Time.hpp>
#include <PrivacyFilter/Filter.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserActionProfile.hpp>

#include "Compatibility/UserActionProfileAdapter.hpp"
#include "UserActionInfoContainer.hpp"

namespace
{
  const Generics::Time TIME_SYNC_PRECISION(2); // 2 sec
  const Generics::Time WAIT_ACTION_EXPIRE_TIME = Generics::Time::ONE_HOUR;

  const unsigned long MAX_DONE_IMPRESSIONS = 100;
  const unsigned long MAX_KEEP_CUSTOM_ACTION_MARKERS = 100;
  const Generics::Time DONE_IMPRESSIONS_EXPIRE_TIME = Generics::Time::ONE_DAY * 30;
  const Generics::Time CUSTOM_ACTION_MARKER_EXPIRE_TIME = Generics::Time::ONE_DAY * 7;
}

namespace Aspect
{
  const char USER_ACTION_INFO_CONTAINER[] = "UserActionInfoContainer";
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  struct DoneActionReaderLessVal
  {
    DoneActionReaderLessVal(unsigned long action_id_val, unsigned long time_val)
      : action_id(action_id_val),
        time(time_val)
    {}

    unsigned long action_id;
    unsigned long time;
  };
  
  struct DoneActionReaderLess
  {
    bool operator()(
      const DoneActionReader& left,
      const DoneActionReaderLessVal& right)
      const
    {
      return left.action_id() < right.action_id ||
        ( left.action_id() == right.action_id &&
          left.time() < right.time);
    }
  };

  struct ActionMarkerLess
  {
    struct Key
    {
      Key(unsigned long ccg_id_val, unsigned long cc_id_val)
          : ccg_id(ccg_id_val), cc_id(cc_id_val)
      {}

      unsigned long ccg_id;
      unsigned long cc_id;
    };

    bool operator()(
      const ActionMarkerReader& left,
      unsigned long right)
      const
    {
      return left.ccg_id() < right;
    }

    bool operator()(
      unsigned long left,
      const ActionMarkerReader& right)
      const
    {
      return left < right.ccg_id();
    }
    
    bool operator()(const ActionMarkerReader& left, const Key& right) const
    {
      return left.ccg_id() < right.ccg_id ||
        (left.ccg_id() == right.ccg_id &&
        left.cc_id() < right.cc_id);
    }

    bool operator()(const Key& left, const ActionMarkerReader& right) const
    {
      return left.ccg_id < right.ccg_id() ||
        (left.ccg_id == right.ccg_id() &&
        left.cc_id < right.cc_id());
    }
  };

  struct CustomActionMarkerLess
  {
    struct Key
    {
      Key(const Generics::Time& time_val, unsigned long action_id_val)
        : time(time_val.tv_sec),
          action_id(action_id_val)
      {}

      unsigned long time;
      unsigned long action_id;
    };
    
    bool
    operator()(const CustomActionMarkerReader& left, const Key& right) const
    {
      return left.time() < right.time ||
        (left.time() == right.time &&
        left.action_id() < right.action_id);
    }

    bool operator()(const Key& left, const CustomActionMarkerReader& right) const
    {
      return left.time < right.time() ||
        (left.time == right.time() &&
        left.action_id < right.action_id());
    }
  };

  struct DoneImpressionLess
  {
    bool
    operator()(const DoneImpressionReader& left, unsigned long right)
      const
    {
      return left.ccg_id() < right;
    }

    bool
    operator()(unsigned long left, const DoneImpressionReader& right)
      const
    {
      return left < right.ccg_id();
    }    
  };

  static bool
  check_action_time_bounds_(
    const UserActionProfileReader::custom_done_actions_Container& done_actions,
    const AdvActionProcessor::AdvExActionInfo& adv_action_info,
    const Generics::Time& action_ignore_time)
  {
    unsigned long timeout = action_ignore_time.tv_sec;
    bool ignore_action = false;
        
    if(!done_actions.empty())
    {
      unsigned long action_id = adv_action_info.action_id;
      
      UserActionProfileReader::custom_done_actions_Container::const_iterator
        act_it = std::lower_bound(
          done_actions.begin(),
          done_actions.end(),
          DoneActionReaderLessVal(
            action_id,
            adv_action_info.time.tv_sec),
          DoneActionReaderLess());

      UserActionProfileReader::custom_done_actions_Container::const_iterator
        prev_act_it = done_actions.end();

      if(act_it != done_actions.end())
      {
        if((*act_it).action_id() == action_id &&
           (unsigned long)std::abs(
             (long)(*act_it).time() - adv_action_info.time.tv_sec) <
           timeout)
        {
          ignore_action = true;
        }

        if(act_it != done_actions.begin())
        {
          prev_act_it = act_it;
          --prev_act_it;
        }
      }
      else
      {
        prev_act_it = --done_actions.end();
      }

      if(prev_act_it != done_actions.end())
      {
        if((*prev_act_it).action_id() == action_id &&
           (unsigned long)std::abs(
             (long)(*prev_act_it).time() - adv_action_info.time.tv_sec) <
           timeout)
        {
          ignore_action = true;
        }
      }
    }

    return ignore_action;
  }
  
  static void
  insert_done_action_(
    UserActionProfileWriter::custom_done_actions_Container& done_actions,
    const AdvActionProcessor::AdvExActionInfo& adv_action_info)
  {
    unsigned long action_id = adv_action_info.action_id;

    UserActionProfileWriter::custom_done_actions_Container::iterator
      ins_it = done_actions.begin();
      
    for(; ins_it != done_actions.end(); ++ins_it)
    {
      if(action_id < ins_it->action_id() ||
         (action_id == ins_it->action_id() &&
         static_cast<unsigned long>(
           adv_action_info.time.tv_sec) < ins_it->time()))
      {
        break;
      }
    }

    DoneActionWriter new_writer;
    new_writer.action_id() = action_id;
    new_writer.time() = adv_action_info.time.tv_sec;
    done_actions.insert(ins_it, new_writer);
  }

  static void
  update_action_marker(
    UserActionProfileWriter::action_markers_Container& action_markers,
    const char* request_id,
    const RequestInfo& request_info,
    const UserActionProfileReader::action_markers_Container*
      read_action_markers)
    /*throw(eh::Exception)*/
  {
    size_t action_marker_pos = 0;

    if(read_action_markers)
    {
      action_marker_pos = std::lower_bound(
        read_action_markers->begin(),
        read_action_markers->end(),
        ActionMarkerLess::Key(request_info.ccg_id, request_info.cc_id),
        ActionMarkerLess()) -
        read_action_markers->begin();
    }

    std::list<ActionMarkerWriter>::iterator it =
      action_markers.begin();

    if(read_action_markers)
    {
      std::advance(it, action_marker_pos);
    }

    if(it != action_markers.end() &&
       it->ccg_id() == request_info.ccg_id &&
       it->cc_id() == request_info.cc_id)
    {
      /* if exists - link marker to more fresh request */
      if(request_info.time > Generics::Time(it->time()))
      {
        it->request_id() = request_id;
      }
    }
    else
    {
      ActionMarkerWriter new_act_marker;
      new_act_marker.ccg_id() = request_info.ccg_id;
      new_act_marker.cc_id() = request_info.cc_id;
      new_act_marker.request_id() = request_id;
      new_act_marker.time() = request_info.time.tv_sec;
      action_markers.insert(it, new_act_marker);
    }
  }

  static void
  process_click_for_at1(
    unsigned long& delegate_process_actions,
    UserActionProfileWriter& user_profile_writer,
    const RequestInfo& request_info,
    const UserActionProfileReader* user_profile_reader)
    /*throw(eh::Exception)*/
  {
    Generics::Time check_low_req_time(request_info.time - TIME_SYNC_PRECISION);

    UserActionProfileReader::action_markers_Container act_markers;

    if(user_profile_reader)
    {
      act_markers = user_profile_reader->action_markers();
    }
    
    update_action_marker(
      user_profile_writer.action_markers(),
      request_info.request_id.to_string().c_str(),
      request_info,
      user_profile_reader ? &act_markers : 0);

    /* search wait actions - action tracking I */
    std::list<WaitActionWriter>::iterator wit =
      user_profile_writer.wait_actions().begin();

    while(wit != user_profile_writer.wait_actions().end() &&
      wit->ccg_id() < request_info.ccg_id)
    {
      ++wit;
    }

    delegate_process_actions = 0;

    while(wit != user_profile_writer.wait_actions().end() &&
       wit->ccg_id() == request_info.ccg_id)
    {
      if(Generics::Time(wit->time()) >= check_low_req_time)
      {
        delegate_process_actions += wit->count();
        wit = user_profile_writer.wait_actions().erase(wit);
      }
      else
      {
        ++wit;
      }
    }
  }

  static void
  process_request_for_at2(
    AdvCustomActionInfoList& delegate_process_custom_actions,
    UserActionProfileWriter& user_profile_writer,
    const RequestInfo& request_info,
    const UserActionProfileReader* user_profile_reader,
    bool insert_request)
    /*throw(eh::Exception)*/
  {
    // find actions that done after this impression and delegate it
    if(user_profile_reader)
    {
      Generics::Time check_req_time(request_info.imp_time - TIME_SYNC_PRECISION);

      UserActionProfileReader::custom_action_markers_Container::
        const_iterator wit = std::lower_bound(
          user_profile_reader->custom_action_markers().begin(),
          user_profile_reader->custom_action_markers().end(),
          CustomActionMarkerLess::Key(check_req_time, 0),
          CustomActionMarkerLess());

      for(; wit != user_profile_reader->custom_action_markers().end(); ++wit)
      {
        bool found = std::binary_search(
            (*wit).ccg_ids().begin(),
            (*wit).ccg_ids().end(),
            request_info.ccg_id);
        
        if(found)
        {
          AdvCustomActionInfo new_custom_action;
          new_custom_action.action_id = (*wit).action_id();
          new_custom_action.action_request_id =
            AdServer::Commons::UserId((*wit).action_request_id());
          new_custom_action.referer = (*wit).referer();
          new_custom_action.time = Generics::Time((*wit).time());
          new_custom_action.order_id = (*wit).order_id();
          new_custom_action.action_value = RevenueDecimal((*wit).action_value());

          delegate_process_custom_actions.push_back(new_custom_action);
        }
      }
    }

    if(insert_request)
    {
      // insert request and clean excess
      UserActionProfileWriter::done_impressions_Container::iterator
        imp_it = user_profile_writer.done_impressions().begin();

      while(imp_it != user_profile_writer.done_impressions().end() &&
        imp_it->ccg_id() < request_info.ccg_id)
      {
        ++imp_it;
      }

      unsigned long imp_count = 0;
      UserActionProfileWriter::done_impressions_Container::iterator
        ccg_first_imp_it = imp_it;

      while(imp_it != user_profile_writer.done_impressions().end() &&
        imp_it->ccg_id() == request_info.ccg_id &&
        imp_it->time() < request_info.imp_time.tv_sec)
      {
        ++imp_it;
        ++imp_count;
      }

      DoneImpressionWriter done_impression_writer;
      done_impression_writer.ccg_id() = request_info.ccg_id;
      done_impression_writer.time() = request_info.imp_time.tv_sec;
      done_impression_writer.request_id() = request_info.request_id.to_string();
      user_profile_writer.done_impressions().insert(
        imp_it, done_impression_writer);
      ++imp_count;

      while(imp_it != user_profile_writer.done_impressions().end() &&
        imp_it->ccg_id() == request_info.ccg_id)
      {
        ++imp_it;
        ++imp_count;
      }

      if(imp_count > MAX_DONE_IMPRESSIONS)
      {
        user_profile_writer.done_impressions().erase(ccg_first_imp_it);
      }
    }
  }

  static void
  clear_expired_at2(
    UserActionProfileWriter& user_profile_writer,
    const Generics::Time& now)
  {
    const unsigned long imp_expire_time = (
      now - DONE_IMPRESSIONS_EXPIRE_TIME).tv_sec;

    for(UserActionProfileWriter::done_impressions_Container::iterator imp_it =
          user_profile_writer.done_impressions().begin();
        imp_it != user_profile_writer.done_impressions().end();)
    {
      if(imp_it->time() < imp_expire_time)
      {
        user_profile_writer.done_impressions().erase(imp_it++);
      }
      else
      {
        ++imp_it;
      }
    }

    const unsigned long act_expire_time = (
      now - CUSTOM_ACTION_MARKER_EXPIRE_TIME).tv_sec;

    UserActionProfileWriter::custom_action_markers_Container::iterator
      am_erase_begin_it =
        user_profile_writer.custom_action_markers().begin();
    UserActionProfileWriter::custom_action_markers_Container::iterator
      am_erase_end_it = am_erase_begin_it;

    for(; am_erase_end_it != user_profile_writer.custom_action_markers().end();
        ++am_erase_end_it)
    {
      if(am_erase_end_it->time() >= act_expire_time)
      {
        break;
      }
    }

    user_profile_writer.custom_action_markers().erase(
      am_erase_begin_it, am_erase_end_it);
  }

  class UserActionInfoContainer::RequestActionProcessorImpl:
    public virtual RequestActionProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    RequestActionProcessorImpl(UserActionInfoContainer* owner)
      : owner_(owner)
    {}

    virtual void
    process_request(
      const RequestInfo&,
      const ProcessingState&)
      /*throw(Exception)*/
    {}

    virtual void
    process_action(const RequestInfo&)
      /*throw(Exception)*/
    {}

    virtual void
    process_impression(
      const RequestInfo& ri,
      const ImpressionInfo& imp_info,
      const ProcessingState& processing_state)
      /*throw(Exception)*/
    {
      owner_->process_impression_(ri, imp_info, processing_state);
    }

    virtual void
    process_click(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(Exception)*/
    {
      owner_->process_click_(ri, processing_state);
    }
    
  private:
    UserActionInfoContainer* owner_;
  };
  
  UserActionInfoContainer::UserActionInfoContainer(
    Logging::Logger* logger,
    RequestContainerProcessor* request_container_processor,
    const char* useractionfile_base_path,
    const char* useractionfile_prefix,
    const Generics::Time& action_ignore_time,
    ProfilingCommons::ProfileMapFactory::Cache* cache,
    const Generics::Time& expire_time,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      action_ignore_time_(action_ignore_time),
      expire_time_(expire_time),
      request_container_processor_(
        ReferenceCounting::add_ref(request_container_processor)),
      request_processor_(new RequestActionProcessorImpl(this))
  {
    static const char* FUN = "UserActionInfoContainer::UserActionInfoContainer()";

    Generics::Time extend_time_period_val(extend_time_period);

    if(extend_time_period_val == Generics::Time::ZERO)
    {
      extend_time_period_val = std::max(expire_time / 4, Generics::Time(1));
    }

    try
    {
      user_map_ = ProfilingCommons::ProfileMapFactory::
        open_transaction_expire_map<
          Commons::UserId,
          ProfilingCommons::UserIdAccessor,
          UserActionProfileAdapter>(
          useractionfile_base_path,
          useractionfile_prefix,
          extend_time_period_val,
          UserActionProfileAdapter(),
          cache);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init UserActionInfoMap. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  UserActionInfoContainer::~UserActionInfoContainer() noexcept
  {}

  RequestActionProcessor_var
  UserActionInfoContainer::request_processor() noexcept
  {
    return request_processor_;
  }

  Generics::ConstSmartMemBuf_var
  UserActionInfoContainer::get_profile(
    const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserActionInfoContainer::get_profile()";
    try
    {
      return user_map_->get_profile(user_id);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get profile for id = " << user_id <<
        ". Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserActionInfoContainer::process_impression_(
    const RequestInfo& request_info,
    const ImpressionInfo&,
    const RequestActionProcessor::ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    static const char* FUN = "UserActionInfoContainer::process_impression_()";

    if(request_info.user_id == AdServer::Commons::PROBE_USER_ID ||
      request_info.user_id == OPTOUT_USER_ID ||
      request_info.user_id.is_null() ||
      processing_state.state != RequestInfo::RS_NORMAL ||
      !request_info.has_custom_actions)
    {
      return;
    }

    AdvCustomActionInfoList delegate_process_custom_actions;

    process_impression_trans_(
      delegate_process_custom_actions,
      request_info);

    /* delegate processing of actions by action tracking II */
    for(AdvCustomActionInfoList::const_iterator
          it = delegate_process_custom_actions.begin();
        it != delegate_process_custom_actions.end(); ++it)
    {
      try
      {
        request_container_processor_->process_custom_action(
          request_info.request_id, *it);
      }
      catch(const RequestContainerProcessor::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught RequestContainerProcessor::Exception: " <<
          ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Processed action marker: " << std::endl;
      request_info.print(ostr, "  ");

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_ACTION_INFO_CONTAINER);
    }
  }

  void
  UserActionInfoContainer::process_click_(
    const RequestInfo& request_info,
    const RequestActionProcessor::ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    static const char* FUN = "UserActionInfoContainer::process_click_()";

    if(request_info.user_id == AdServer::Commons::PROBE_USER_ID ||
      request_info.user_id == OPTOUT_USER_ID ||
      request_info.user_id.is_null() ||
      processing_state.state != RequestInfo::RS_NORMAL ||
      (!request_info.enabled_action_tracking && !request_info.has_custom_actions))
    {
      return;
    }

    unsigned long delegate_process_actions;
    AdvCustomActionInfoList delegate_process_custom_actions;

    process_click_trans_(
      delegate_process_actions,
      delegate_process_custom_actions,
      request_info);

    /* delegate processing of actions by action tracking I */
    for(unsigned long i = 0; i < delegate_process_actions; ++i)
    {
      try
      {
        request_container_processor_->process_action(
          RequestContainerProcessor::AT_ACTION,
          Generics::Time::get_time_of_day(),
          request_info.request_id);
      }
      catch(const RequestContainerProcessor::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught RequestContainerProcessor::Exception: " <<
          ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    for(AdvCustomActionInfoList::const_iterator
          it = delegate_process_custom_actions.begin();
        it != delegate_process_custom_actions.end(); ++it)
    {
      try
      {
        request_container_processor_->process_custom_action(
          request_info.request_id, *it);
      }
      catch(const RequestContainerProcessor::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught RequestContainerProcessor::Exception: " <<
          ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Processed action marker: " << std::endl;
      request_info.print(ostr, "  ");

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_ACTION_INFO_CONTAINER);
    }
  }

  void
  UserActionInfoContainer::process_adv_action(
    const AdvActionInfo& adv_action_info)
    /*throw(AdvActionProcessor::Exception)*/
  {
    static const char* FUN = "UserActionInfoContainer::process_adv_action()";
    
    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": To process adv action: " << std::endl;
      adv_action_info.print(ostr, "  ");

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_ACTION_INFO_CONTAINER);
    }

    RequestIdList delegate_process_actions;

    process_adv_action_trans_(
      delegate_process_actions,
      adv_action_info);

    for(RequestIdList::const_iterator req_it =
          delegate_process_actions.begin();
        req_it != delegate_process_actions.end(); ++req_it)
    {
      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": Action done: " << std::endl;
        adv_action_info.print(ostr, "  ");

        logger_->log(
          ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_ACTION_INFO_CONTAINER);
      }

      try
      {
        request_container_processor_->process_action(
          RequestContainerProcessor::AT_ACTION, 
          Generics::Time::get_time_of_day(),
          *req_it);
      }
      catch(const RequestContainerProcessor::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught RequestContainerProcessor::Exception: " <<
          ex.what();
        throw AdvActionProcessor::Exception(ostr);
      }
    }
    
    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Processed adv action: " << std::endl;
      adv_action_info.print(ostr, "  ");

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_ACTION_INFO_CONTAINER);
    }
  }

  void
  UserActionInfoContainer::process_custom_action(
    const AdvExActionInfo& adv_ex_action_info)
    /*throw(AdvActionProcessor::Exception)*/
  {
    static const char* FUN = "UserActionInfoContainer::process_custom_action()";

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": To process adv custom action: " << std::endl;
      adv_ex_action_info.print(ostr, "  ");

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_ACTION_INFO_CONTAINER);
    }

    if(adv_ex_action_info.user_id.is_null())
    {
      return;
    }

    DelegateCustomActionInfoList delegate_custom_actions;

    process_custom_action_trans_(
      delegate_custom_actions,
      adv_ex_action_info);

    for(DelegateCustomActionInfoList::const_iterator act_it =
          delegate_custom_actions.begin();
        act_it != delegate_custom_actions.end(); ++act_it)
    {
      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": Custom action done: " << std::endl;
        act_it->print(ostr, "  ");

        logger_->log(
          ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_ACTION_INFO_CONTAINER);
      }

      try
      {
        request_container_processor_->process_custom_action(
            act_it->request_id, *act_it);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << ex.what();
        throw AdvActionProcessor::Exception(ostr);
      }
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Processed adv custom action: " << std::endl;
      adv_ex_action_info.print(ostr, "  ");

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_ACTION_INFO_CONTAINER);
    }
  }

  void
  UserActionInfoContainer::clear_expired_actions()
    /*throw(Exception)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    user_map_->clear_expired(now - expire_time_);
  }

  void
  UserActionInfoContainer::process_impression_trans_(
    AdvCustomActionInfoList& delegate_process_custom_actions,
    const RequestInfo& request_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    static const char* FUN = "UserActionInfoContainer::process_impression_trans_()";

    try
    {
      Generics::Time check_req_time(request_info.imp_time - TIME_SYNC_PRECISION);

      UserActionProfileWriter user_profile_writer;
      std::unique_ptr<UserActionProfileReader> user_profile_reader;

      UserActionInfoMap::Transaction_var transaction =
        user_map_->get_transaction(request_info.user_id);

//    std::cout << request_info.user_id << " opened" << std::endl;

      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

      if(mem_buf.in())
      {
        user_profile_writer.init(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        user_profile_reader.reset(
          new UserActionProfileReader(
            mem_buf->membuf().data(),
            mem_buf->membuf().size()));
      }
      else
      {
        user_profile_writer.version() = CURRENT_ACTION_INFO_PROFILE_VERSION;
      }

      process_request_for_at2(
        delegate_process_custom_actions,
        user_profile_writer,
        request_info,
        user_profile_reader.get(),
        true // insert request
        );

      clear_expired_at2(user_profile_writer, request_info.imp_time);

      // save profile
      unsigned long sz = user_profile_writer.size();
      Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));

      user_profile_writer.save(new_mem_buf->membuf().data(), sz);

      transaction->save_profile(
        Generics::transfer_membuf(new_mem_buf),
        request_info.imp_time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw RequestActionProcessor::Exception(ostr);
    }
  }

  void
  UserActionInfoContainer::process_click_trans_(
    unsigned long& delegate_process_actions,
    AdvCustomActionInfoList& delegate_process_custom_actions,
    const RequestInfo& request_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    static const char* FUN = "UserActionInfoContainer::process_click_trans_()";

    delegate_process_actions = 0;

    try
    {
      UserActionProfileWriter user_profile_writer;
      std::unique_ptr<UserActionProfileReader> user_profile_reader;

      UserActionInfoMap::Transaction_var transaction =
        user_map_->get_transaction(request_info.user_id);

      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

      if(mem_buf.in())
      {
        user_profile_writer.init(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        user_profile_reader.reset(
          new UserActionProfileReader(
            mem_buf->membuf().data(),
            mem_buf->membuf().size()));
      }
      else
      {
        user_profile_writer.version() = CURRENT_ACTION_INFO_PROFILE_VERSION;
      }

      if(request_info.enabled_action_tracking)
      {
        process_click_for_at1(
          delegate_process_actions,
          user_profile_writer,
          request_info,
          user_profile_reader.get());
      }

      if(request_info.has_custom_actions)
      {
        process_request_for_at2(
          delegate_process_custom_actions,
          user_profile_writer,
          request_info,
          user_profile_reader.get(),
          false // don't insert request
          );
      }

      clear_expired_at2(user_profile_writer, request_info.imp_time);

      // save profile
      unsigned long sz = user_profile_writer.size();
      Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));

      user_profile_writer.save(new_mem_buf->membuf().data(), sz);

      transaction->save_profile(
        Generics::transfer_membuf(new_mem_buf),
        request_info.imp_time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw RequestActionProcessor::Exception(ostr);
    }
  }

  void
  UserActionInfoContainer::process_adv_action_trans_(
    RequestIdList& delegate_process_actions,
    const AdvActionInfo& adv_action_info)
    /*throw(AdvActionProcessor::Exception)*/
  {
    static const char* FUN = "UserActionInfoContainer::process_adv_action_trans_()";

    try
    {
      bool found = false;

      UserActionInfoMap::Transaction_var transaction =
        user_map_->get_transaction(adv_action_info.user_id);

      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

      if(mem_buf.in())
      {
        // try find marker
        UserActionProfileReader user_profile_reader(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        // call processor for confirmed action
        UserActionProfileReader::action_markers_Container::const_iterator it =
          std::lower_bound(
            user_profile_reader.action_markers().begin(),
            user_profile_reader.action_markers().end(),
            adv_action_info.ccg_id,
            ActionMarkerLess());

        UserActionProfileReader::action_markers_Container::const_iterator max_it =
          user_profile_reader.action_markers().end();

        for(; it != user_profile_reader.action_markers().end() &&
              (*it).ccg_id() == adv_action_info.ccg_id; ++it)
        {
          if(max_it == user_profile_reader.action_markers().end() ||
             (*it).time() > (*max_it).time())
          {
            max_it = it;
          }
        }

        if(max_it != user_profile_reader.action_markers().end())
        {
          delegate_process_actions.push_back(
            AdServer::Commons::RequestId(
              (*max_it).request_id()));

          found = true;
        }
      }

      if(!found)
      {
        // add wait action and save profile
        UserActionProfileWriter user_profile_writer;

        if(mem_buf.in())
        {
          user_profile_writer.init(
            mem_buf->membuf().data(),
            mem_buf->membuf().size());
        }
        else
        {
          user_profile_writer.version() = CURRENT_ACTION_INFO_PROFILE_VERSION;
        }

        std::list<WaitActionWriter>::iterator wit =
          user_profile_writer.wait_actions().begin();

        while(wit != user_profile_writer.wait_actions().end() &&
           wit->ccg_id() < adv_action_info.ccg_id)
        {
          ++wit;
        }

        if(wit != user_profile_writer.wait_actions().end() &&
           wit->ccg_id() == adv_action_info.ccg_id)
        {
          /* clear excess wait actions (time < current action time - WAIT_ACTION_EXPIRE_TIME) */
          const Generics::Time clear_actions_time =
            adv_action_info.time - WAIT_ACTION_EXPIRE_TIME;

          std::list<WaitActionWriter>::iterator erase_begin = wit;

          while(wit != user_profile_writer.wait_actions().end() &&
            wit->ccg_id() == adv_action_info.ccg_id &&
            wit->time() < clear_actions_time.tv_sec)
          {
            ++wit;
          }

          user_profile_writer.wait_actions().erase(erase_begin, wit);
        }

        while(wit != user_profile_writer.wait_actions().end() &&
          wit->ccg_id() == adv_action_info.ccg_id &&
          wit->time() < adv_action_info.time.tv_sec)
        {
          ++wit;
        }

        if(wit != user_profile_writer.wait_actions().end() &&
          wit->ccg_id() == adv_action_info.ccg_id &&
          wit->time() == adv_action_info.time.tv_sec)
        {
          wit->count()++;
        }
        else
        {
          WaitActionWriter wait_action;
          wait_action.ccg_id() = adv_action_info.ccg_id;
          wait_action.count() = 1;
          wait_action.time() = adv_action_info.time.tv_sec;
          user_profile_writer.wait_actions().insert(wit, wait_action);
        }

        unsigned long sz = user_profile_writer.size();
        Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));

        user_profile_writer.save(new_mem_buf->membuf().data(), sz);

        transaction->save_profile(
          Generics::transfer_membuf(new_mem_buf),
          adv_action_info.time);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw AdvActionProcessor::Exception(ostr);
    }
  }

  void
  UserActionInfoContainer::process_custom_action_trans_(
    DelegateCustomActionInfoList& delegate_custom_actions,
    const AdvExActionInfo& adv_ex_action_info)
    /*throw(AdvActionProcessor::Exception)*/
  {
    static const char* FUN =
      "UserActionInfoContainer::process_custom_action_trans_()";

    try
    {
      bool ignore_action = false;

      UserActionInfoMap::Transaction_var transaction =
        user_map_->get_transaction(adv_ex_action_info.user_id);

      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

      if(mem_buf.in())
      {
        UserActionProfileReader user_profile_reader(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        // check action time bound
        ignore_action = check_action_time_bounds_(
          user_profile_reader.custom_done_actions(),
          adv_ex_action_info,
          action_ignore_time_);

        if(!ignore_action)
        {
          Generics::Time check_low_req_time(
            adv_ex_action_info.time + TIME_SYNC_PRECISION);

          DelegateCustomActionInfo base_adv_custom_action_info;
          base_adv_custom_action_info.time = adv_ex_action_info.time;
          base_adv_custom_action_info.action_id = adv_ex_action_info.action_id;
          base_adv_custom_action_info.action_request_id =
            adv_ex_action_info.action_request_id;
          base_adv_custom_action_info.referer = adv_ex_action_info.referer;
          base_adv_custom_action_info.order_id = adv_ex_action_info.order_id;
          base_adv_custom_action_info.action_value = adv_ex_action_info.action_value;

          for(AdvExActionInfo::CCGIdList::const_iterator ccg_it =
                adv_ex_action_info.ccg_ids.begin();
              ccg_it != adv_ex_action_info.ccg_ids.end(); ++ccg_it)
          {
            UserActionProfileReader::done_impressions_Container::
              const_iterator imp_it = std::lower_bound(
                user_profile_reader.done_impressions().begin(),
                user_profile_reader.done_impressions().end(),
                *ccg_it,
                DoneImpressionLess());

            while(imp_it != user_profile_reader.done_impressions().end() &&
              (*imp_it).ccg_id() == *ccg_it &&
              (*imp_it).time() <= check_low_req_time.tv_sec)
            {
              DelegateCustomActionInfo adv_custom_action_info(
                base_adv_custom_action_info);
              adv_custom_action_info.request_id =
                AdServer::Commons::RequestId((*imp_it).request_id());
              delegate_custom_actions.push_back(adv_custom_action_info);
              ++imp_it;
            }
          }
        }
      }

      if(!ignore_action)
      {
        UserActionProfileWriter user_profile_writer;

        if(mem_buf.in())
        {
          user_profile_writer.init(
            mem_buf->membuf().data(),
            mem_buf->membuf().size());
        }
        else
        {
          user_profile_writer.version() = CURRENT_ACTION_INFO_PROFILE_VERSION;
        }

        {
          // insert new custom action marker
          UserActionProfileWriter::custom_action_markers_Container::iterator wit =
            user_profile_writer.custom_action_markers().begin();

          while(wit != user_profile_writer.custom_action_markers().end() &&
            wit->time() < adv_ex_action_info.time.tv_sec)
          {
            ++wit;
          }

          while(wit != user_profile_writer.custom_action_markers().end() &&
            wit->time() == adv_ex_action_info.time.tv_sec &&
            wit->action_id() < adv_ex_action_info.action_id)
          {
            ++wit;
          }

          CustomActionMarkerWriter action_marker_writer;
          action_marker_writer.action_id() = adv_ex_action_info.action_id;
          action_marker_writer.action_request_id() =
            adv_ex_action_info.action_request_id.to_string();
          action_marker_writer.time() = adv_ex_action_info.time.tv_sec;
          action_marker_writer.referer() = adv_ex_action_info.referer;
          action_marker_writer.order_id() = adv_ex_action_info.order_id;
          action_marker_writer.action_value() = adv_ex_action_info.action_value.str();
          std::copy(adv_ex_action_info.ccg_ids.begin(),
            adv_ex_action_info.ccg_ids.end(),
            std::back_inserter(action_marker_writer.ccg_ids()));

          user_profile_writer.custom_action_markers().insert(wit, action_marker_writer);
        }

        {
          // clear excess action_id marker
          unsigned long am_count = 0;
          for(UserActionProfileWriter::custom_action_markers_Container::
                reverse_iterator wit =
                  user_profile_writer.custom_action_markers().rbegin();
              wit != user_profile_writer.custom_action_markers().rend(); )
          {
            if(wit->action_id() == adv_ex_action_info.action_id)
            {
              if(am_count >= MAX_KEEP_CUSTOM_ACTION_MARKERS)
              {
                UserActionProfileWriter::custom_action_markers_Container::
                  reverse_iterator tmp = wit;
                user_profile_writer.custom_action_markers().erase((++tmp).base());
              }
              else
              {
                ++am_count;
                ++wit;
              }
            }
            else
            {
              ++wit;
            }
          }
        }

        insert_done_action_(
          user_profile_writer.custom_done_actions(),
          adv_ex_action_info);

        clear_expired_at2(user_profile_writer, adv_ex_action_info.time);

        unsigned long sz = user_profile_writer.size();
        Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));

        user_profile_writer.save(new_mem_buf->membuf().data(), sz);

        transaction->save_profile(
          Generics::transfer_membuf(new_mem_buf),
          adv_ex_action_info.time);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw AdvActionProcessor::Exception(ostr);
    }
  }
} /* namespace RequestInfoSvcs */
} /* namespace AdServer */
