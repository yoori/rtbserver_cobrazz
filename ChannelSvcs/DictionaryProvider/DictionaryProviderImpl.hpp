#ifndef AD_SERVER_CHANNEL_DICTIONARY_PROVIDER_IMPL_HPP_
#define AD_SERVER_CHANNEL_DICTIONARY_PROVIDER_IMPL_HPP_

#include <set>
#include <vector>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Sync/Semaphore.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelCommons.hpp>
#include <ChannelSvcs/DictionaryProvider/DictionaryProvider_s.hpp>

#include <xsd/ChannelSvcs/DictionaryProviderConfig.hpp>
#include "DictionaryContainer.hpp"

namespace AdServer
{
  namespace ChannelSvcs
  {
    /**
     * Implementation of common part DictionaryProvider
     */
    class DictionaryProviderImpl:
      public virtual Generics::CompositeActiveObject,
      public virtual CORBACommons::ReferenceCounting::ServantImpl
       <POA_AdServer::ChannelSvcs::DictionaryProvider>
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      typedef xsd::AdServer::Configuration::DictionaryProviderConfigType
        DictionaryProviderConfig;

      DictionaryProviderImpl(
        Logging::Logger* logger,
        const DictionaryProviderConfig* config)
        /*throw(Exception, eh::Exception)*/;

      virtual ~DictionaryProviderImpl() noexcept {};

      void load_dictionary_(
        const char* lang,
        const char* file_name) noexcept;

    public:

      virtual
        ::AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq*
        get_lexemes(
          const char * language,
          const ::CORBACommons::StringSeq& words)
        /*throw(ChannelSvcs::NotReady,
              ChannelSvcs::ImplementationException)*/;
      
    protected:
      typedef Generics::TaskGoal TaskBase;
      typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

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
      volatile _Atomic_word task_runned_;
      static const char* ASPECT;
      static const size_t MAX_LOAD_SIZE;
    };

    typedef ReferenceCounting::SmartPtr<DictionaryProviderImpl>
        DictionaryProviderImpl_var;

  } /* ChannelSvcs */
} /* AdServer */

namespace AdServer
{
  namespace ChannelSvcs
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

  } /* ChannelSvcs */
} /* AdServer */


#endif /*AD_SERVER_CHANNEL_DICTIONARY_PROVIDER_IMPL_HPP_*/
