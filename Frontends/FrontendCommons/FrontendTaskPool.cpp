#include "FrontendTaskPool.hpp"

namespace FrontendCommons
{
  // FrontendTaskPool::HandleRequestTask
  class FrontendTaskPool::HandleRequestTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    HandleRequestTask(
      FrontendTaskPool* frontend_task_pool,
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
      bool noparams)
      : frontend_task_pool_(frontend_task_pool),
        http_request_(std::move(request_holder)),
        response_writer_(std::move(response_writer)),
        noparams_(noparams)
    {}

    virtual void
    execute() noexcept
    {
      if(noparams_)
      {
        frontend_task_pool_->handle_request_noparams_(
          std::move(http_request_),
          std::move(response_writer_));
      }
      else
      {
        frontend_task_pool_->handle_request_(
          std::move(http_request_),
          std::move(response_writer_));
      }

      frontend_task_pool_->task_count_ += -1;
    }

  protected:
    virtual
    ~HandleRequestTask() noexcept = default;

  protected:
    FrontendTaskPool* frontend_task_pool_;
    FrontendCommons::HttpRequestHolder_var http_request_;
    FrontendCommons::HttpResponseWriter_var response_writer_;
    const bool noparams_;
  };

  // FrontendTaskPool impl
  FrontendTaskPool::FrontendTaskPool(
    Generics::ActiveObjectCallback* callback,
    FrontendCommons::HttpResponseFactory* response_factory,
    unsigned long threads,
    unsigned long max_pending_tasks)
    /*throw(eh::Exception)*/
    : FrontendInterface(response_factory),
      threads_(threads),
      max_pending_tasks_(max_pending_tasks),
      task_count_(0)
  {
    task_runner_ = new Generics::TaskRunner(callback, threads);
    //task_runner_ = new Generics::TaskRunner(callback_holder_.callback(), threads);
    add_child_object(task_runner_);
  }

  void
  FrontendTaskPool::push_handle_request_task_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer,
    bool noparams)
    noexcept
  {
    unsigned long cur_task_count = task_count_.exchange_and_add(1) + 1;
    if(max_pending_tasks_ > 0 && cur_task_count > max_pending_tasks_ + threads_)
    {
      task_count_ += -1;

      // TODO: move to virtual method
      // default processing
      response_writer->write(204, create_response());
    }
    else
    {
      Generics::Task_var request_task = new HandleRequestTask(
        this,
        std::move(request_holder),
        std::move(response_writer),
        noparams);

      task_runner_->enqueue_task(request_task);
    }
  }

  void
  FrontendTaskPool::handle_request(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    noexcept
  {
    push_handle_request_task_(std::move(request_holder), std::move(response_writer), false);
  }

  void
  FrontendTaskPool::handle_request_noparams(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    noexcept
  {
    push_handle_request_task_(std::move(request_holder), std::move(response_writer), true);
  }

  void
  FrontendTaskPool::handle_request_noparams_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    noexcept
  {
    if(parse_args_(request_holder, response_writer))
    {
      handle_request_(std::move(request_holder), std::move(response_writer));
    }
  }
}

