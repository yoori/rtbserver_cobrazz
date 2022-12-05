#ifndef ADSERVER_FRONTENDS_UIDGENERATORADAPTER_UIDGENERATORADAPTERLOGGER_HPP_
#define ADSERVER_FRONTENDS_UIDGENERATORADAPTER_UIDGENERATORADAPTERLOGGER_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <LogCommons/LogHolder.hpp>
#include <LogCommons/KeywordHitStat.hpp>

namespace AdServer
{
namespace Frontends
{
  class UIDGeneratorAdapterLogger:
    public virtual AdServer::LogProcessing::CompositeLogHolder
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::vector<AdServer::Commons::StringHolder_var> KeywordArray;

    struct RequestInfo
    {
      Generics::Time time;
      KeywordArray keywords;
    };

    class RequestLogger;

  public:
    UIDGeneratorAdapterLogger(
      Logging::Logger* logger,
      const AdServer::LogProcessing::LogFlushTraits& keyword_hit_stat_log_params)
        /*throw(Exception)*/;

    void
    process_request(
      const RequestInfo& request_info)
      /*throw(Exception)*/;

  protected:
    typedef ReferenceCounting::SmartPtr<RequestLogger>
      RequestLogger_var;

    typedef std::list<RequestLogger_var> RequestLoggerList;

  protected:
    virtual
    ~UIDGeneratorAdapterLogger() noexcept;

  protected:
    Logging::Logger_var logger_;
    RequestLoggerList request_loggers_;
  };

  typedef ReferenceCounting::SmartPtr<UIDGeneratorAdapterLogger>
    UIDGeneratorAdapterLogger_var;
}
}

#endif /*ADSERVER_FRONTENDS_UIDGENERATORADAPTER_UIDGENERATORADAPTERLOGGER_HPP_*/
