#include <sstream>

#include <Generics/Time.hpp>
#include <PrivacyFilter/Filter.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserFraudProtectionProfile.hpp>

#include "UserFraudProtectionContainer.hpp"

namespace Aspect
{
  const char USER_FRAUD_PROTECTION_CONTAINER[] = "UserFraudProtectionContainer";
}

namespace
{
  const unsigned long CURRENT_USER_FRAUD_PROTECTION_PROFILE_VERSION = 1;
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    struct UserMotionReaderTimeLess
    {
      bool
      operator()(const UserMotionReader& left, const Generics::Time& right)
        const noexcept
      {
        return Generics::Time(left.time()) < right;
      }

      bool
      operator()(const Generics::Time& left, const UserMotionReader& right)
        const noexcept
      {
        return left < Generics::Time(right.time());
      }
    };
      
    void
    add_user_motion(
      UserFraudProtectionProfileWriter::requests_Container& motions,
      const AdServer::Commons::RequestId& request_id,
      const Generics::Time& time)
    {
      UserFraudProtectionProfileWriter::requests_Container::reverse_iterator mit =
        motions.rbegin();
      
      for(; mit != motions.rend(); ++mit)
      {
        if(mit->time() <= time.tv_sec)
        {
          break;
        }
      }
      
      std::string rid_str = request_id.to_string();
          
      UserMotionWriter new_motion;
      new_motion.request_id() = rid_str;
      new_motion.time() = time.tv_sec;
      motions.insert(mit.base(), new_motion);
    }
    
    void
    clear_excess_motions(
      UserFraudProtectionProfileWriter::requests_Container& motions,
      const Generics::Time& max_time)
    {
      UserFraudProtectionProfileWriter::requests_Container::iterator mit =
        motions.begin();
      
      for(; mit != motions.end(); ++mit)
      {
        if(mit->time() >= max_time.tv_sec)
        {
          break;
        }
      }

      motions.erase(motions.begin(), mit);
    }

    void
    pop_rollback_requests(
      UserFraudProtectionContainer::RequestIdList& rollback_requests,
      UserFraudProtectionProfileWriter::requests_Container& motions,
      const Generics::Time& min_time,
      const Generics::Time& max_time)
    {
      UserFraudProtectionProfileWriter::requests_Container::
        reverse_iterator end_it = motions.rbegin();

      for(; end_it != motions.rend(); ++end_it)
      {
        if(Generics::Time(end_it->time()) <= max_time)
        {
          break;
        }
      }
      
      UserFraudProtectionProfileWriter::requests_Container::
        reverse_iterator rit = end_it;
      
      for(; rit != motions.rend(); ++rit)
      {
        if(Generics::Time(rit->time()) < min_time)
        {
          break;
        }

        rollback_requests.push_back(
          AdServer::Commons::RequestId(rit->request_id().c_str()));
      }

      motions.erase(rit.base(), end_it.base());
    }

    bool
    user_motions_is_fraud(
      Generics::Time& fraud_start_time,
      Generics::Time& fraud_end_time,
      const Generics::Time& now,
      unsigned long now_motions,
      const UserFraudProtectionProfileReader::requests_Container& motions,
      const UserFraudProtectionContainer::Config::FraudRuleSet& fraud_rules)
    {
      bool is_fraud = false;

      const Generics::Time checkall_start_time(now - fraud_rules.max_period());

      UserFraudProtectionProfileReader::requests_Container::const_iterator
        checkall_start_it = std::lower_bound(
          motions.begin(),
          motions.end(),
          checkall_start_time,
          UserMotionReaderTimeLess());

      UserFraudProtectionProfileReader::requests_Container::const_iterator
        checkall_end_it = std::lower_bound(
          checkall_start_it,
          motions.end(),
          now + fraud_rules.max_period(),
          UserMotionReaderTimeLess());

      // find up and low points for "now"
      UserFraudProtectionProfileReader::requests_Container::const_iterator
        now_up_it = std::upper_bound(
          checkall_start_it,
          checkall_end_it,
          now,
          UserMotionReaderTimeLess());

      UserFraudProtectionProfileReader::requests_Container::const_iterator
        now_low_it = std::lower_bound(
          checkall_start_it,
          now_up_it,
          now,
          UserMotionReaderTimeLess());

      for(UserFraudProtectionContainer::Config::FraudRuleList::const_iterator
            rule_it = fraud_rules.rules().begin();
          rule_it != fraud_rules.rules().end(); ++rule_it)
      {
        bool local_is_fraud = false;

        UserFraudProtectionProfileReader::requests_Container::const_iterator
          check_start_it = std::lower_bound(
            checkall_start_it,
            now_low_it,
            Generics::Time(now - rule_it->period),
            UserMotionReaderTimeLess());

        UserFraudProtectionProfileReader::requests_Container::const_iterator
          check_end_it = std::upper_bound(
            now_up_it,
            checkall_end_it,
            Generics::Time(now + rule_it->period),
            UserMotionReaderTimeLess());

        // optimization precheck:
        // if number of actions inside [now - period, now + period]
        // less then limit fraud rule can't work
        if(check_end_it - check_start_it + now_motions >= rule_it->limit)
        {
          // point inside fraud interval
          // truncate points that need to check by limit
          if(static_cast<unsigned long>(now_low_it - check_start_it) + 1 >
               rule_it->limit)
          {
            check_start_it = now_low_it + (1 - rule_it->limit);
          }

          if(static_cast<unsigned long>(check_end_it - now_up_it) >
             rule_it->limit)
          {
            check_end_it = now_up_it + rule_it->limit;
          }

          // search minimum(with maximal start point) fraud interval
          // by moving period interval [check_start_it, right_it]
          // from past to future
          for(UserFraudProtectionProfileReader::requests_Container::const_iterator
                right_it = check_start_it;
              check_start_it != check_end_it && right_it != check_end_it; )
          {
            if(rule_it->period + (*check_start_it).time() <
               Generics::Time((*right_it).time()))
            {
              ++check_start_it;
            }
            else if(right_it - check_start_it + 1 + now_motions >= rule_it->limit)
            {
              const Generics::Time check_start_it_time(
                (*check_start_it).time());
              const Generics::Time fraud_min_time(
                now < check_start_it_time ? now : check_start_it_time);

              fraud_start_time = (
                fraud_start_time == Generics::Time::ZERO ?
                fraud_min_time :
                std::min(fraud_start_time, fraud_min_time));

              fraud_end_time = std::max(
                fraud_end_time,
                fraud_min_time + rule_it->period);

              local_is_fraud = true;

              break;
            }
            else
            {
              ++right_it;
            }
          }

          // search maximum fraud interval
          // by moving period interval [left_it, check_end_it]
          // from future to past
          if(local_is_fraud && check_start_it != check_end_it)
          {
            --check_end_it;

            for(UserFraudProtectionProfileReader::requests_Container::
                  const_iterator left_it = check_end_it;
                check_end_it != check_start_it &&
                  left_it != check_start_it; )
            {
              if(rule_it->period + (*left_it).time() <
                   Generics::Time((*check_end_it).time()))
              {
                --check_end_it;
              }
              else if(check_end_it - left_it + 1 + now_motions >=
                rule_it->limit)
              {
                Generics::Time left_it_time((*left_it).time());

                // current request is left bound
                fraud_end_time = std::max(
                  fraud_end_time,
                  (now < left_it_time ? now : left_it_time) + rule_it->period);                  

                break;
              }
              else
              {
                --left_it;
              }
            }
          }
        }

        is_fraud |= local_is_fraud;
      }

      return is_fraud;
    }
  }

  //
  // UserFraudProtectionContainer
  //
  UserFraudProtectionContainer::UserFraudProtectionContainer(
    Logging::Logger* logger,
    RequestContainerProcessor* request_container_processor,
    Callback* callback,
    const char* file_base_path,
    const char* file_prefix,
    ProfilingCommons::ProfileMapFactory::Cache* cache,
    const Generics::Time& expire_time,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      expire_time_(expire_time),
      request_container_processor_(ReferenceCounting::add_ref(request_container_processor)),
      callback_(ReferenceCounting::add_ref(callback))
  {
    static const char* FUN = "UserFraudProtectionContainer::UserFraudProtectionContainer()";

    Generics::Time extend_time_period_val(extend_time_period);
    
    if(extend_time_period_val == Generics::Time::ZERO)
    {
      extend_time_period_val = expire_time / 4;
    }

    try
    {
      user_map_ = AdServer::ProfilingCommons::ProfileMapFactory::
        open_transaction_expire_map<
          AdServer::Commons::UserId,
          ProfilingCommons::UserIdAccessor>(
          file_base_path,
          file_prefix,
          extend_time_period_val,
          cache);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init ProfileMap. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  UserFraudProtectionContainer::~UserFraudProtectionContainer() noexcept
  {}

  Generics::ConstSmartMemBuf_var
  UserFraudProtectionContainer::get_profile(
    const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserFraudProtectionContainer::get_profile()";
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

  void UserFraudProtectionContainer::process_impression(
    const RequestInfo& request_info,
    const ImpressionInfo&,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    static const char* FUN = "UserFraudProtectionContainer::process_impression()";
    
    if(request_info.user_id == AdServer::Commons::PROBE_USER_ID ||
      request_info.user_id == OPTOUT_USER_ID ||
      request_info.user_id.is_null() ||
      processing_state.state != RequestInfo::RS_NORMAL ||
      request_info.disable_fraud_detection)
    {
      return;
    }

    RequestIdList fraud_impressions;
    Generics::Time user_deactivate_time;

    process_impression_trans_(
      fraud_impressions,
      user_deactivate_time,
      request_info);

    try
    {
      for(RequestIdList::const_iterator it = fraud_impressions.begin();
          it != fraud_impressions.end(); ++it)
      {
        request_container_processor_->process_action(
          RequestContainerProcessor::AT_FRAUD_ROLLBACK,
          Generics::Time::get_time_of_day(),
          *it);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw RequestActionProcessor::Exception(ostr);
    }
    
    if(user_deactivate_time != Generics::Time::ZERO &&
       callback_.in())
    {
      callback_->detected_fraud_user(request_info.user_id, user_deactivate_time);
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Processed impression: " << std::endl;
      request_info.print(ostr, "  ");

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_FRAUD_PROTECTION_CONTAINER);
    }
  }

  void
  UserFraudProtectionContainer::process_click(
    const RequestInfo& request_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    static const char* FUN = "UserFraudProtectionContainer::process_click_()";

    if(request_info.user_id == AdServer::Commons::PROBE_USER_ID ||
      request_info.user_id == OPTOUT_USER_ID ||
      request_info.user_id.is_null() ||
      processing_state.state != RequestInfo::RS_NORMAL ||
      request_info.disable_fraud_detection)
    {
      return;
    }

    /*
      ", state = ";
    RequestInfo::print_request_state(std::cerr, processing_state.state);
    std::cerr << std::endl;
    */
    /*
      ", state = ";
    RequestInfo::print_request_state(std::cerr, processing_state.state);
    std::cerr << std::endl;
    */
    RequestIdList fraud_impressions;
    RequestIdList fraud_clicks;
    Generics::Time user_deactivate_time;

    process_click_trans_(
      fraud_impressions,
      user_deactivate_time,
      request_info);

    try
    {
      for(RequestIdList::const_iterator it = fraud_impressions.begin();
          it != fraud_impressions.end(); ++it)
      {
        request_container_processor_->process_action(
          RequestContainerProcessor::AT_FRAUD_ROLLBACK,
          Generics::Time::get_time_of_day(),
          *it);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw RequestActionProcessor::Exception(ostr);
    }

    if(user_deactivate_time != Generics::Time::ZERO &&
       callback_.in())
    {
      callback_->detected_fraud_user(request_info.user_id, user_deactivate_time);
    }
    
    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Processed click: " << std::endl;
      request_info.print(ostr, "  ");

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_FRAUD_PROTECTION_CONTAINER);      
    }
  }

  void
  UserFraudProtectionContainer::process_impression_trans_(
    RequestIdList& fraud_impressions,
    Generics::Time& user_deactivate_time,
    const RequestInfo& request_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    static const char* FUN = "UserFraudProtectionContainer::process_impression_trans_()";

    user_deactivate_time = Generics::Time::ZERO;
    
    Config_var config = get_config_();

    try
    {
      UserFraudProtectionProfileWriter user_profile_writer;
      std::unique_ptr<UserFraudProtectionProfileReader> user_profile_reader;

      ProfileMap::Transaction_var transaction =
        user_map_->get_transaction(request_info.user_id);
      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();
  
      if(mem_buf.in())
      {
        user_profile_writer.init(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        user_profile_reader.reset(
          new UserFraudProtectionProfileReader(
            mem_buf->membuf().data(),
            mem_buf->membuf().size()));
      }
      else
      {
        user_profile_writer.version() = CURRENT_USER_FRAUD_PROTECTION_PROFILE_VERSION;
        user_profile_writer.fraud_time() = 0;
      }

      Generics::Time fraud_start_time;
      Generics::Time fraud_end_time;

      bool fraud = request_info.time <
        Generics::Time(user_profile_writer.fraud_time());

      if(config.in())
      {
        if(user_profile_reader.get() &&
           request_info.position == 1)
        {
          fraud |= user_motions_is_fraud(
            fraud_start_time,
            fraud_end_time,
            request_info.time,
            1,
            user_profile_reader->requests(),
            config->imp_rules);
        }

        clear_excess_motions(
          user_profile_writer.requests(),
          request_info.time - config->imp_rules.max_period());
        clear_excess_motions(
          user_profile_writer.rollback_requests(),
          request_info.time - std::max(
            config->imp_rules.max_period(), config->click_rules.max_period()));
        clear_excess_motions(
          user_profile_writer.clicks(),
          request_info.time - config->click_rules.max_period());
      }

      if(request_info.position == 1)
      {
        add_user_motion(user_profile_writer.requests(),
          request_info.request_id, request_info.time);
      }

      if(!fraud)
      {
        add_user_motion(user_profile_writer.rollback_requests(),
          request_info.request_id, request_info.time);
      }
      else
      {
        fraud_impressions.push_back(request_info.request_id);

        if(fraud_end_time != Generics::Time::ZERO)
        {
          user_deactivate_time = fraud_end_time +
            config->deactivate_period;
          user_profile_writer.fraud_time() =
            user_deactivate_time.tv_sec;
        }

        pop_rollback_requests(fraud_impressions,
          user_profile_writer.rollback_requests(),
          fraud_start_time,
          user_deactivate_time);
      }

      /* save profile */
      unsigned long sz = user_profile_writer.size();
      Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));
  
      user_profile_writer.save(new_mem_buf->membuf().data(), sz);
  
      transaction->save_profile(
        Generics::transfer_membuf(new_mem_buf),
        request_info.time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw RequestActionProcessor::Exception(ostr);
    }
  }

  void
  UserFraudProtectionContainer::process_click_trans_(
    RequestIdList& fraud_impressions,
    Generics::Time& user_deactivate_time,
    const RequestInfo& request_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    static const char* FUN = "UserFraudProtectionContainer::process_click_trans_()";

    user_deactivate_time = Generics::Time::ZERO;
    
    Config_var config = get_config_();

    try
    {
      UserFraudProtectionProfileWriter user_profile_writer;
      std::unique_ptr<UserFraudProtectionProfileReader> user_profile_reader;

      ProfileMap::Transaction_var transaction =
        user_map_->get_transaction(request_info.user_id);
      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

      if(mem_buf.in())
      {
        user_profile_writer.init(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        user_profile_reader.reset(
          new UserFraudProtectionProfileReader(
            mem_buf->membuf().data(),
            mem_buf->membuf().size()));
      }
      else
      {
        user_profile_writer.version() = CURRENT_USER_FRAUD_PROTECTION_PROFILE_VERSION;
        user_profile_writer.fraud_time() = 0;
      }

      Generics::Time fraud_start_time;
      Generics::Time fraud_end_time;
      bool fraud = request_info.click_time <
        Generics::Time(user_profile_writer.fraud_time());

      if(config.in())
      {
        if(user_profile_reader.get() && !fraud)
        {
          fraud |= user_motions_is_fraud(
            fraud_start_time,
            fraud_end_time,
            request_info.time,
            1,
            user_profile_reader->clicks(),
            config->click_rules);
        }
      
        clear_excess_motions(
          user_profile_writer.requests(),
          request_info.time - config->imp_rules.max_period());
        clear_excess_motions(
          user_profile_writer.rollback_requests(),
          request_info.time - std::max(
            config->imp_rules.max_period(), config->click_rules.max_period()));
        clear_excess_motions(
          user_profile_writer.clicks(),
          request_info.time - config->click_rules.max_period());
      }

      if(fraud)
      {
        if(fraud_end_time != Generics::Time::ZERO)
        {
          user_deactivate_time = fraud_end_time +
            config->deactivate_period;
          user_profile_writer.fraud_time() =
            user_deactivate_time.tv_sec;
        }

        pop_rollback_requests(
          fraud_impressions,
          user_profile_writer.rollback_requests(),
          fraud_start_time,
          user_deactivate_time);
      }

      add_user_motion(user_profile_writer.clicks(),
        request_info.request_id, request_info.time);
      
      /* save profile */
      unsigned long sz = user_profile_writer.size();
      Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));
  
      user_profile_writer.save(new_mem_buf->membuf().data(), sz);
  
      transaction->save_profile(
        Generics::transfer_membuf(new_mem_buf),
        request_info.time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw RequestActionProcessor::Exception(ostr);
    }
  }

  void UserFraudProtectionContainer::clear_expired()
    /*throw(Exception)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    user_map_->clear_expired(now - expire_time_);
  }
} /* namespace RequestInfoSvcs */
} /* namespace AdServer */
