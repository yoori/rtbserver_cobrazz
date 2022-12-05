/**
 * @file PassbackContainer.hpp
 */
#ifndef REQUESTINFOSVCS_PASSBACKCONTAINER_HPP
#define REQUESTINFOSVCS_PASSBACKCONTAINER_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>

#include "TagRequestProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    struct PassbackProcessor: public virtual ReferenceCounting::Interface
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct PassbackInfo
      {
        // verified passback info
        PassbackInfo()
          : colo_id(0), tag_id(0), size_id(0)
        {}

        bool operator==(const PassbackInfo& right) const;

        std::ostream& print(std::ostream& ostr, const char* prefix) const;

        char user_status;
        Generics::Time time;
        unsigned long colo_id;
        std::string country;
        unsigned long tag_id;
        unsigned long size_id;
        std::string ext_tag_id;
        std::string referer;
      };

      virtual void
      process_passback(const PassbackInfo& passback_info)
        /*throw(Exception)*/ = 0;

    protected:
      virtual ~PassbackProcessor() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<PassbackProcessor> PassbackProcessor_var;

    struct PassbackVerificationProcessor:
      public virtual ReferenceCounting::Interface
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      virtual void
      process_passback_request(
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& impression_time)
        /*throw(Exception)*/ = 0;

    protected:
      virtual ~PassbackVerificationProcessor() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<PassbackVerificationProcessor>
      PassbackVerificationProcessor_var;

    /** PassbackContainer
     * merge input passback requests:
     *  call next process_passback only if called
     *  process_passback, process_passback_request (passback considering)
     *  ignore duplicate passback considering.
     */
    class PassbackContainer:
      public virtual TagRequestProcessor,
      public virtual PassbackVerificationProcessor,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      static const Generics::Time DEFAULT_EXPIRE_TIME; // 2 hours

    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      PassbackContainer(
        Logging::Logger* logger,
        PassbackProcessor* passback_processor,
        const char* passbackfile_base_path,
        const char* passbackfile_prefix,
        ProfilingCommons::ProfileMapFactory::Cache* cache,
        const Generics::Time& expire_time =
          Generics::Time(DEFAULT_EXPIRE_TIME),
        const Generics::Time& extend_time_period = Generics::Time::ZERO)
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::RequestId& request_id)
        /*throw(Exception)*/;

      virtual void
      process_tag_request(const TagRequestInfo&)
        /*throw(TagRequestProcessor::Exception)*/;

      virtual void
      process_passback_request(
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& impression_time)
        /*throw(PassbackVerificationProcessor::Exception)*/;

      void clear_expired_requests() /*throw(Exception)*/;

    protected:
      virtual ~PassbackContainer() noexcept {}

    private:
      typedef ProfilingCommons::TransactionProfileMap<
        ProfilingCommons::RequestIdPackHashAdapter>
        PassbackMap;

      typedef ReferenceCounting::SmartPtr<PassbackMap>
        PassbackMap_var;

    private:
      void process_tag_request_trans_(
        PassbackProcessor::PassbackInfo& passback_info,
        bool& delegate_passback_processing,
        const TagRequestInfo& tag_request_info)
        /*throw(TagRequestProcessor::Exception)*/;

    private:
      Logging::Logger_var logger_;
      Generics::Time expire_time_;
      PassbackMap_var passback_map_;
      PassbackProcessor_var passback_processor_;
    };

    typedef ReferenceCounting::SmartPtr<PassbackContainer>
      PassbackContainer_var;
  }
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  inline
  bool
  PassbackProcessor::PassbackInfo::operator==(
    const PassbackInfo& right) const
  {
    return user_status == right.user_status &&
      time == right.time &&
      colo_id == right.colo_id &&
      country == right.country &&
      tag_id == right.tag_id &&
      ext_tag_id == right.ext_tag_id &&
      referer == right.referer;
  }

  inline
  std::ostream&
  PassbackProcessor::PassbackInfo::print(
    std::ostream& ostr, const char* prefix) const
  {
    ostr << prefix << "user_status = '" << user_status << "'" << std::endl <<
      prefix << "time = " << time.get_gm_time() << std::endl <<
      prefix << "colo_id = " << colo_id << std::endl <<
      prefix << "country = " << country << std::endl <<
      prefix << "tag_id = " << tag_id << std::endl <<
      prefix << "size_id = " << size_id << std::endl <<
      prefix << "ext_tag_id = " << ext_tag_id << std::endl <<
      prefix << "referer = " << referer << std::endl;
    return ostr;
  }
}
}

#endif /*REQUESTINFOSVCS_PASSBACKCONTAINER_HPP*/
