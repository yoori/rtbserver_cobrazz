#ifndef FRONTENDCOMMONS_FRONTENDTASKPOOL_HPP_
#define FRONTENDCOMMONS_FRONTENDTASKPOOL_HPP_

#include <String/SubString.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Stream/MemoryStream.hpp>
#include <Generics/AtomicInt.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include "FrontendInterface.hpp"

namespace FrontendCommons
{
  class FrontendTaskPool:
    public virtual FrontendInterface,
    public Generics::CompositeActiveObject
  {
  public:
    FrontendTaskPool(
      Generics::ActiveObjectCallback* callback,
      FrontendCommons::HttpResponseFactory* response_factory,
      unsigned long threads,
      unsigned long max_pending_tasks)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept = 0;

    virtual void
    handle_request(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
      noexcept;

    virtual void
    handle_request_noparams(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
      noexcept;

  protected:
    class HandleRequestTask;

  protected:
    virtual
    ~FrontendTaskPool() noexcept = default;

    virtual void
    handle_request_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
      noexcept = 0;

    virtual void
    handle_request_noparams_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
      noexcept;

    void
    push_handle_request_task_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
      bool noparams)
      noexcept;

  protected:
    const unsigned long threads_;
    const unsigned long max_pending_tasks_;

    //Logging::LoggerCallbackHolder callback_holder_;
    Generics::TaskRunner_var task_runner_;
    Generics::AtomicInt task_count_;
  };
}

#endif /*FRONTENDCOMMONS_FRONTENDTASKPOOL_HPP_*/
