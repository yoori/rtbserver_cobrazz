#pragma once

#include <atomic>
#include <set>
#include <vector>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Sync/Semaphore.hpp>

#include <ChannelSvcs/DictionaryProvider/DictionaryProvider.grpc.pb.h>

#include <xsd/ChannelSvcs/DictionaryProviderConfig.hpp>
#include "DictionaryContainer.hpp"

namespace AdServer::ChannelSvcs
{
  /**
   * Implementation of common part DictionaryProvider
   */
  class DictionaryProviderImpl:
    public virtual Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl,
    public adserver::channel_svcs::dictionary_provider::DictionaryProvider::Service
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using DictionaryProviderConfig = xsd::AdServer::Configuration::DictionaryProviderConfigType;

    DictionaryProviderImpl(Logging::Logger* logger, const DictionaryProviderConfig* config)
      /*throw(Exception, eh::Exception)*/;

    ~DictionaryProviderImpl() noexcept override;

    void load_dictionary_(const char* lang, const char* file_name) noexcept;

  public:

    grpc::Status get_lexemes(
      grpc::ServerContext* context,
      const adserver::channel_svcs::dictionary_provider::GetLexemesRequest* request,
      adserver::channel_svcs::dictionary_provider::GetLexemesResponse* response) override;

  protected:
    using TaskBase = Generics::TaskGoal;
    using Task_var = ReferenceCounting::SmartPtr<TaskBase>;

    class LoadTask: public TaskBase
    {
    public:

      LoadTask(
        DictionaryProviderImpl* impl,
        Generics::TaskRunner* task_runner,
        const char* file_name,
        const char* lang)
        noexcept;

      virtual ~LoadTask() noexcept;
      virtual void execute() noexcept;

    private:
      DictionaryProviderImpl* impl_;
      const char* lang_;
      const char* file_name_;
    };


    Logging::Logger* logger () const noexcept;

  private:
    Logging::ActiveObjectCallbackImpl_var callback_;
    Generics::TaskRunner_var task_runner_;
    DictionaryContainer cont_;
    std::atomic<long> task_runned_;
    static const char* ASPECT;
    static const size_t MAX_LOAD_SIZE;
  };

  using DictionaryProviderImpl_var = ReferenceCounting::SmartPtr<DictionaryProviderImpl>;

} // namespace AdServer::ChannelSvcs

namespace AdServer::ChannelSvcs
{
  /* DictionaryProviderImpl */
  inline
  Logging::Logger*
  DictionaryProviderImpl::logger() const noexcept
  {
    return callback_->logger();
  }

  inline
  DictionaryProviderImpl::LoadTask::LoadTask(
    DictionaryProviderImpl* impl,
    Generics::TaskRunner* task_runner,
    const char* file_name,
    const char* lang)
    noexcept
    : TaskBase(task_runner),
      impl_(impl),
      lang_(lang),
      file_name_(file_name)
  {
  }

  inline
  DictionaryProviderImpl::LoadTask::~LoadTask() noexcept
  {
  }

  inline void
  DictionaryProviderImpl::LoadTask::execute() noexcept
  {
    impl_->load_dictionary_(lang_, file_name_);
  }

} // namespace AdServer::ChannelSvcs
