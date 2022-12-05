#ifndef ADSERVER_REQUESTINFOSVCS_REQUESTINFOMANAGER_EXPRESSIONMATCHERNOTIFIER_HPP
#define ADSERVER_REQUESTINFOSVCS_REQUESTINFOMANAGER_EXPRESSIONMATCHERNOTIFIER_HPP

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>

#include <LogCommons/LogHolder.hpp>
#include <ProfilingCommons/MessageSaver.hpp>

#include "RequestActionProcessor.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  /** ExpressionMatcherNotifier */
  class ExpressionMatcherNotifier:
    public virtual RequestActionProcessor,
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    typedef RequestActionProcessor::Exception Exception;

  public:
    ExpressionMatcherNotifier(
      Logging::Logger* logger,
      bool notify_impressions,
      bool notify_revenue,
      unsigned long distrib_count,
      const LogProcessing::LogFlushTraits& action_saver_opts,
      const LogProcessing::LogFlushTraits& click_saver_opts,
      const LogProcessing::LogFlushTraits& impression_saver_opts,
      const LogProcessing::LogFlushTraits& request_saver_opts)
      noexcept;

    virtual void
    process_request(
      const RequestInfo&,
      const ProcessingState&)
      /*throw(Exception)*/
    {}

    virtual void
    process_impression(
      const RequestInfo&,
      const ImpressionInfo&,
      const ProcessingState& processing_state)
      /*throw(Exception)*/;

    virtual void
    process_click(
      const RequestInfo&,
      const ProcessingState& processing_state)
      /*throw(Exception)*/;

    virtual void
    process_action(const RequestInfo&) /*throw(Exception)*/;

  protected:
    virtual
    ~ExpressionMatcherNotifier() noexcept
    {}

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;

  private:
    static bool
    need_process_(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      noexcept;

  private:
    Logging::Logger_var logger_;
    const bool notify_impressions_;
    const bool notify_revenue_;

    ProfilingCommons::MessageSaver_var action_saver_;
    ProfilingCommons::MessageSaver_var click_saver_;
    ProfilingCommons::MessageSaver_var impression_saver_;
    ProfilingCommons::MessageSaver_var request_saver_;
  };

  typedef ReferenceCounting::SmartPtr<ExpressionMatcherNotifier>
    ExpressionMatcherNotifier_var;
}
}

#endif /*ADSERVER_REQUESTINFOSVCS_REQUESTINFOMANAGER_EXPRESSIONMATCHERNOTIFIER_HPP*/
