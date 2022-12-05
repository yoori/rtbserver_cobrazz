/**
 * @file PassbackContainer.cpp
 */
#include <eh/Exception.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/PassbackProfile.hpp>

#include "Compatibility/PassbackProfileAdapter.hpp"
#include "PassbackContainer.hpp"

namespace Aspect
{
  const char PASSBACK_CONTAINER[] = "PassbackContainer";
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  const Generics::Time
  PassbackContainer::DEFAULT_EXPIRE_TIME = Generics::Time::ONE_HOUR * 2;

  PassbackContainer::PassbackContainer(
    Logging::Logger* logger,
    PassbackProcessor* passback_processor,
    const char* passbackfile_base_path,
    const char* passbackfile_prefix,
    ProfilingCommons::ProfileMapFactory::Cache* cache,
    const Generics::Time& expire_time,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      expire_time_(expire_time),
      passback_processor_(ReferenceCounting::add_ref(passback_processor))
  {
    static const char* FUN = "PassbackContainer::PassbackContainer()";
    
    Generics::Time extend_time_period_val(extend_time_period);

    if(extend_time_period_val == Generics::Time::ZERO)
    {
      extend_time_period_val = expire_time / 4;
    }

    try
    {
      passback_map_ = ProfilingCommons::ProfileMapFactory::
        open_transaction_packed_expire_map<
          ProfilingCommons::RequestIdPackHashAdapter,
          ProfilingCommons::RequestIdAccessor,
          PassbackProfileAdapter>(
          passbackfile_base_path,
          passbackfile_prefix,
          extend_time_period_val,
          cache);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init PassbackMap. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  Generics::ConstSmartMemBuf_var
  PassbackContainer::get_profile(
    const AdServer::Commons::RequestId& request_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "PassbackContainer::get_profile()";
    try
    {
      return passback_map_->get_profile(request_id);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get_ profile for id = " << request_id <<
        ". Caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void
  PassbackContainer::process_tag_request(
    const TagRequestInfo& tag_request_info)
    /*throw(TagRequestProcessor::Exception)*/
  {
    static const char* FUN = "PassbackContainer::process_tag_request()";

    if(!tag_request_info.request_id.is_null() &&
       tag_request_info.tag_id)
    {
      PassbackProcessor::PassbackInfo passback_info;
      bool delegate_passback_processing;

      process_tag_request_trans_(
        passback_info,
        delegate_passback_processing,
        tag_request_info);

      if(delegate_passback_processing)
      {
        try
        {
          passback_processor_->process_passback(passback_info);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught eh::Exception: " << ex.what();
          throw PassbackProcessor::Exception(ostr);
        }
      }

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": Processed passback marker: " << std::endl;
        tag_request_info.print(ostr, "  ");

        logger_->log(ostr.str(),
          Logging::Logger::TRACE,
          Aspect::PASSBACK_CONTAINER);
      }
    }
  }

  void
  PassbackContainer::process_tag_request_trans_(
    PassbackProcessor::PassbackInfo& passback_info,
    bool& delegate_passback_processing,
    const TagRequestInfo& tag_request_info)
    /*throw(TagRequestProcessor::Exception)*/
  {
    static const char* FUN = "PassbackContainer::process_tag_request_trans_()";

    delegate_passback_processing = false;

    try
    {
      PassbackMap::Transaction_var transaction =
        passback_map_->get_transaction(tag_request_info.request_id);

      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

      bool save_required = true;
      PassbackInfoWriter passback_writer;

      if(mem_buf.in())
      {
        passback_writer.init(mem_buf->membuf().data(), mem_buf->membuf().size());

        if(passback_writer.verified() && !passback_writer.done())
        {
          /* delegate call */
          delegate_passback_processing = true;
          passback_info.time = tag_request_info.time;
          passback_info.user_status = tag_request_info.user_status;
          passback_info.colo_id = tag_request_info.colo_id;
          passback_info.country = tag_request_info.country;
          passback_info.tag_id = tag_request_info.tag_id;
          passback_info.size_id = tag_request_info.size_id;
          passback_info.ext_tag_id = tag_request_info.ext_tag_id;
          passback_info.referer = tag_request_info.referer;
          // Impression time (logs read in order Impr + Opp)
          // stored in profile,/ save Opportunity time to profile
          // and save Impression time to passback information.
          passback_info.time = Generics::Time(passback_writer.time());

          passback_writer.done() = 1;
          passback_writer.time() = tag_request_info.time.tv_sec;

          // save passback traits - only for debug
          passback_writer.user_status() = tag_request_info.user_status;
          passback_writer.tag_id() = tag_request_info.tag_id;
          passback_writer.size_id() = tag_request_info.size_id;
          passback_writer.ext_tag_id() = tag_request_info.ext_tag_id;
          passback_writer.colo_id() = tag_request_info.colo_id;
          passback_writer.country() = tag_request_info.country;
          passback_writer.referer() = tag_request_info.referer;
        }
        else
        {
          // duplicated request
          save_required = false;
        }
      }
      else
      {
        passback_writer.version() = CURRENT_PASSBACK_PROFILE_VERSION;
        passback_writer.request_id() = tag_request_info.request_id.to_string();
        passback_writer.user_status() = tag_request_info.user_status;
        passback_writer.tag_id() = tag_request_info.tag_id;
        passback_writer.size_id() = tag_request_info.size_id;
        passback_writer.ext_tag_id() = tag_request_info.ext_tag_id;
        passback_writer.colo_id() = tag_request_info.colo_id;
        passback_writer.country() = tag_request_info.country;
        passback_writer.time() = tag_request_info.time.tv_sec;
        passback_writer.referer() = tag_request_info.referer;
        passback_writer.done() = 0;
        passback_writer.verified() = 0;
      }

      if(save_required)
      {
        /* save profile */
        unsigned long sz = passback_writer.size();
        Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));
        passback_writer.save(new_mem_buf->membuf().data(), sz);
        transaction->save_profile(
          Generics::transfer_membuf(new_mem_buf),
          tag_request_info.time);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw TagRequestProcessor::Exception(ostr);
    }
  }

  void
  PassbackContainer::process_passback_request(
    const AdServer::Commons::RequestId& request_id,
    const Generics::Time& impression_time)
    /*throw(PassbackVerificationProcessor::Exception)*/
  {
    static const char* FUN = "PassbackContainer::process_passback_request()";

    try
    {
      bool process_passback = false;
      PassbackProcessor::PassbackInfo process_passback_info;

      {
        PassbackMap::Transaction_var transaction =
          passback_map_->get_transaction(request_id);
        Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

        PassbackInfoWriter passback_writer;

        if(mem_buf.in())
        {
          passback_writer.init(
            mem_buf->membuf().data(),
            mem_buf->membuf().size());

          if(!passback_writer.verified())
          {
            /* delegate call */
            process_passback = true;
            process_passback_info.user_status = passback_writer.user_status();
            // Already got Opportunity, set Impression time into PassbackStat
            process_passback_info.time = impression_time;
            process_passback_info.colo_id = passback_writer.colo_id();
            process_passback_info.country = passback_writer.country();
            process_passback_info.tag_id = passback_writer.tag_id();
            process_passback_info.size_id = passback_writer.size_id();
            process_passback_info.ext_tag_id = passback_writer.ext_tag_id();
            process_passback_info.referer = passback_writer.referer();

            passback_writer.verified() = 1;
          }
        }
        else
        {
          /* create wait stub */
          passback_writer.version() = CURRENT_PASSBACK_PROFILE_VERSION;
          passback_writer.request_id() = request_id.to_string();
          passback_writer.user_status() = '-';
          passback_writer.tag_id() = 0;
          passback_writer.size_id() = 0;
          passback_writer.colo_id() = 0;
          // Not yet received Opportunity, temporary store Impression time
          // in profile,  opportunity handler will exchange times
          passback_writer.time() = impression_time.tv_sec;
          passback_writer.done() = 0;
          passback_writer.verified() = 1;
        }

        /* save profile */
        unsigned long sz = passback_writer.size();
        Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));

        passback_writer.save(new_mem_buf->membuf().data(), sz);

        transaction->save_profile(
          Generics::transfer_membuf(new_mem_buf),
          impression_time);
      }

      if(process_passback)
      {
        passback_processor_->process_passback(process_passback_info);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw PassbackVerificationProcessor::Exception(ostr);
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Processed passback request: " << request_id;

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::PASSBACK_CONTAINER);
    }
  }

  void PassbackContainer::clear_expired_requests()
    /*throw(Exception)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    passback_map_->clear_expired(now - expire_time_);
  }
}
}
