/**
 * @file PassbackContainer.hpp
 */
#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Coro/Awaitable.hpp>
#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>

#include "TagRequestProcessor.hpp"

namespace AdServer
{
  namespace ProfilingCommons
  {
    class RocksDBProfileMapProcessor;
  }

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

    using PassbackProcessor_var =
      ReferenceCounting::SmartPtr<PassbackProcessor>;

    struct PassbackVerificationProcessor:
      public virtual ReferenceCounting::Interface
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      virtual void
      process_passback_request(
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& impression_time)
        /*throw(Exception)*/ = 0;

      virtual AdServer::Commons::Awaitable<void>
      co_process_passback_request(
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& impression_time);

    protected:
      virtual ~PassbackVerificationProcessor() noexcept {}
    };

    using PassbackVerificationProcessor_var =
      ReferenceCounting::SmartPtr<PassbackVerificationProcessor>;

    struct RequestIdTransactionHashAdapter:
      public AdServer::Commons::RequestId
    {
      RequestIdTransactionHashAdapter() = default;

      RequestIdTransactionHashAdapter(
        const AdServer::Commons::RequestId& request_id)
        : AdServer::Commons::RequestId(request_id)
      {}

      unsigned long
      hash() const noexcept
      {
        return AdServer::Commons::uuid_distribution_hash(*this) % 100000;
      }
    };

    /** PassbackContainer
     * merge input passback requests:
     *  call next process_passback only if called
     *  process_passback, process_passback_request (passback considering)
     *  ignore duplicate passback considering.
     */
    class PassbackContainer:
      public virtual TagRequestProcessor,
      public virtual PassbackVerificationProcessor,
      public virtual Generics::RefCountableCompositeActiveObject
    {
    public:
      static const Generics::Time DEFAULT_EXPIRE_TIME; // 2 hours

    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      PassbackContainer(
        Logging::Logger* logger,
        PassbackProcessor* passback_processor,
        const char* rocksdb_path,
        const Generics::Time& expire_time =
          Generics::Time(DEFAULT_EXPIRE_TIME),
        std::shared_ptr<ProfilingCommons::RocksDBProfileMapProcessor>
          rocksdb_processor = {})
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::RequestId& request_id)
        /*throw(Exception)*/;

      virtual void
      process_tag_request(const TagRequestInfo&)
        /*throw(TagRequestProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_tag_request(const TagRequestInfo&);

      virtual void
      process_passback_request(
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& impression_time)
        /*throw(PassbackVerificationProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_passback_request(
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& impression_time);

      void clear_expired_requests() /*throw(Exception)*/;

    protected:
      virtual ~PassbackContainer() noexcept {}

    private:
      using PassbackMap = ProfilingCommons::TransactionProfileMap<
        RequestIdTransactionHashAdapter>;

      using PassbackMap_var = ReferenceCounting::SmartPtr<PassbackMap>;

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

    using PassbackContainer_var =
      ReferenceCounting::SmartPtr<PassbackContainer>;
  }
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  inline AdServer::Commons::Awaitable<void>
  PassbackVerificationProcessor::co_process_passback_request(
    const AdServer::Commons::RequestId& request_id,
    const Generics::Time& impression_time)
  {
    process_passback_request(request_id, impression_time);
    co_return;
  }

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
