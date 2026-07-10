#pragma once

#include <eh/Exception.hpp>

#include <Generics/TaskRunner.hpp>
#include <Generics/Values.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>

#include <Generics/TaskRunner.hpp>
#include <Generics/Values.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include "UserInfoContainer.hpp"

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /* BaseOperationRecordFetcher class */
    class BaseOperationRecordFetcher: public LogProcessing::FileProcessor,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      typedef std::set<unsigned long> ChunkIdSet;

      BaseOperationRecordFetcher(
        Generics::ActiveObjectCallback* callback,
        UserInfoContainer* user_info_container,
        const char* folder,
        const char* unprocessed_folder,
        const char* file_prefix,
        const ChunkIdSet& chunk_ids,
        Generics::RefCountableActiveObject* interrupter)
        noexcept;

      virtual void
      process(LogProcessing::FileReceiver::FileGuard* file_ptr) noexcept;

    protected:
      virtual
      ~BaseOperationRecordFetcher() noexcept
      {}

      virtual void
      read_operation_(
        Generics::SmartMemBuf* smart_mem_buf)
        /*throw(eh::Exception)*/ = 0;

    private:
      void
      file_move_back_to_input_dir_(
        const AdServer::LogProcessing::LogFileNameInfo& info,
        const char* file_name) /*throw(eh::Exception)*/;

    protected:
      UserInfoContainer_var user_info_container_;

    private:
      Generics::ActiveObjectCallback_var log_errors_;

      // check configuration
      const std::string DIR_;
      const std::string unprocessed_dir_;
      const std::string file_prefix_;

      const ChunkIdSet chunk_ids_;
      Generics::ActiveObject_var interrupter_;
    };

    typedef ReferenceCounting::AssertPtr<BaseOperationRecordFetcher>::Ptr
      BaseOperationRecordFetcher_var;

    /* InternalOperationRecordFetcher class */
    class InternalOperationRecordFetcher:
      public BaseOperationRecordFetcher
    {
    public:
      InternalOperationRecordFetcher(
        Generics::ActiveObjectCallback* callback,
        UserInfoContainer* user_info_container,
        const char* folder,
        const char* unprocessed_folder,
        const char* file_prefix,
        const ChunkIdSet& chunk_ids,
        Generics::RefCountableActiveObject* interrupter)
        noexcept;

    protected:
      virtual
      ~InternalOperationRecordFetcher() noexcept
      {}

      virtual void
      read_operation_(
        Generics::SmartMemBuf* smart_mem_buf)
        /*throw(eh::Exception)*/;

      void read_fraud_operation_(
        const Generics::MemBuf& mem_buf)
        /*throw(eh::Exception)*/;

      void read_match_operation_(
        Generics::SmartMemBuf* smart_mem_buf)
        /*throw(eh::Exception)*/;

      void read_merge_operation_(
        Generics::SmartMemBuf* smart_mem_buf)
        /*throw(eh::Exception)*/;

      void read_fc_update_operation_(
        Generics::SmartMemBuf* smart_mem_buf)
        /*throw(eh::Exception)*/;

      void read_fc_confirm_operation_(
        Generics::SmartMemBuf* smart_mem_buf)
        /*throw(eh::Exception)*/;

    };
    typedef ReferenceCounting::AssertPtr<InternalOperationRecordFetcher>::Ptr
      InternalOperationRecordFetcher_var;

    /* InternalUserOperationLoader class */
    class InternalUserOperationLoader:
      public virtual Generics::RefCountableCompositeActiveObject
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      InternalUserOperationLoader(
        Generics::ActiveObjectCallback* callback,
        UserInfoContainer* user_info_container,
        const char* operation_file_in_dir,
        const char* unprocessed_dir,
        const char* file_prefix,
        const BaseOperationRecordFetcher::ChunkIdSet& chunk_ids,
        const Generics::Time& check_period,
        std::size_t threads_count)
        /*throw(Exception)*/;

    protected:
      virtual
      ~InternalUserOperationLoader() noexcept;

    private:
      Generics::ActiveObjectCallback_var log_errors_callback_;
      LogProcessing::FileThreadProcessor_var file_thread_processor_;
      InternalOperationRecordFetcher_var operation_fetcher_;
      std::string unprocessed_dir_;
    };

    typedef ReferenceCounting::SmartPtr<InternalUserOperationLoader>
      InternalUserOperationLoader_var;

  }
}
