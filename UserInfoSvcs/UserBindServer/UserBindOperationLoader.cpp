#include <fstream>

#include <eh/Errno.hpp>
#include <Commons/PathManip.hpp>

#include <Generics/DirSelector.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/FileReceiver.hpp>

#include "UserBindOperationProfile.hpp"
#include "UserBindOperationLoader.hpp"

namespace
{
  namespace Aspect
  {
    const char USER_BIND_OPERATION_LOADER[] = "UserBindOperationLoader";
  }

  const char DEFAULT_ERROR_DIR[] = "Error";
  const unsigned long FETCH_FILES_LIMIT = 50000;
}

namespace AdServer
{
namespace UserInfoSvcs
{
  class InterrupterImpl:
    public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
  {};

  // UserBindOperationFetcher
  class UserBindOperationFetcher:
    public LogProcessing::FileProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::set<unsigned long> ChunkIdSet;

    UserBindOperationFetcher(
      Generics::ActiveObjectCallback* callback,
      UserBindProcessor* user_bind_operation_processor,
      const char* folder,
      const char* unprocessed_folder,
      const char* file_prefix,
      const ChunkIdSet& chunk_ids,
      Generics::ActiveObject* interrupter)
      noexcept;

    virtual void
    process(LogProcessing::FileReceiver::FileGuard* file_ptr) noexcept;

  protected:
    virtual
    ~UserBindOperationFetcher() noexcept
    {}

    virtual void
    read_operation_(
      uint32_t operation_type,
      Generics::SmartMemBuf* smart_mem_buf)
      /*throw(eh::Exception)*/;

    virtual void
    read_get_user_id_operation_(
      const void* data_buf,
      unsigned long data_size)
      /*throw(eh::Exception)*/;

    virtual void
    read_add_user_id_operation_(
      const void* data_buf,
      unsigned long data_size)
      /*throw(eh::Exception)*/;

  protected:
    void
    file_move_back_to_input_dir_(
      const AdServer::LogProcessing::LogFileNameInfo& info,
      const char* file_name) /*throw(eh::Exception)*/;

  protected:
    Generics::ActiveObjectCallback_var callback_;

    // check configuration
    const std::string DIR_;
    const std::string unprocessed_dir_;
    const std::string file_prefix_;

    const ChunkIdSet chunk_ids_;
    Generics::ActiveObject_var interrupter_;
    UserBindProcessor_var user_bind_processor_;
  };

  // UserBindOperationFetcher impl
  UserBindOperationFetcher::UserBindOperationFetcher(
    Generics::ActiveObjectCallback* callback,
    UserBindProcessor* user_bind_operation_processor,
    const char* folder,
    const char* unprocessed_folder,
    const char* file_prefix,
    const ChunkIdSet& chunk_ids,
    Generics::ActiveObject* interrupter)
    noexcept
    : callback_(ReferenceCounting::add_ref(callback)),
      DIR_(folder),
      unprocessed_dir_(unprocessed_folder),
      file_prefix_(file_prefix),
      chunk_ids_(chunk_ids),
      interrupter_(ReferenceCounting::add_ref(interrupter)),
      user_bind_processor_(ReferenceCounting::add_ref(user_bind_operation_processor))
  {}

  void
  UserBindOperationFetcher::process(
    LogProcessing::FileReceiver::FileGuard* file_ptr)
    noexcept
  {
    static const char* FUN = "UserBindOperationFetcher::process()";

    // file guard must be destroyed after moving file into errors store
    LogProcessing::FileReceiver::FileGuard_var file(
      ReferenceCounting::add_ref(file_ptr));

    try
    {
      if (!file)
      {
        return;
      }

      Generics::SmartMemBuf_var smart_mem_buf(new Generics::SmartMemBuf());
      Generics::MemBuf& mem_buf = smart_mem_buf.in()->membuf();

      LogProcessing::LogFileNameInfo file_name_info;
      LogProcessing::parse_log_file_name(file->file_name().c_str(), file_name_info);
      unsigned long chunk_id = file_name_info.distrib_index;

      // check, that chunk id controllable
      if(chunk_ids_.find(chunk_id) != chunk_ids_.end())
      {
        std::ifstream file_stream(file->full_path().c_str(), std::ios_base::binary);
        if(file_stream.fail())
        {
          Stream::Error ostr;
          ostr << "Can't open file";
          throw Exception(ostr);
        }

        bool terminated = false;
        unsigned long processed_records = 0;
        unsigned long processed_lines_count = file_name_info.processed_lines_count;

        while(true)
        {
          uint32_t operation_type = 0;
          file_stream.read(reinterpret_cast<char*>(&operation_type), 4);

          uint32_t record_size = 0;
          file_stream.read(reinterpret_cast<char*>(&record_size), 4);

          if(file_stream.eof())
          {
            break;
          }

          if(file_stream.fail())
          {
            Stream::Error ostr;
            ostr << "Reading failed";
            throw Exception(ostr);
          }

          mem_buf.alloc(record_size);
          file_stream.read(mem_buf.get<char>(), mem_buf.size());

          if(file_stream.eof() || file_stream.fail())
          {
            Stream::Error ostr;
            ostr << "Unexpected eof or fail";
            throw Exception(ostr);
          }

          if (processed_records >= file_name_info.processed_lines_count)
          {
            read_operation_(operation_type, smart_mem_buf.in());
            ++processed_lines_count;
          }

          ++processed_records;

          if (interrupter_ && !interrupter_->active())
          {
            terminated = true;
            break;
          }
        }

        file_stream.close();

        if (terminated)
        {
          file_name_info.processed_lines_count = processed_lines_count;
          file_move_back_to_input_dir_(file_name_info, file->full_path().c_str());
        }
        else if(::unlink(file->full_path().c_str()) != 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't delete file '" << file->full_path() << "'";
          callback_->report_error(
            Generics::ActiveObjectCallback::ERROR, ostr.str());
        }
      }
      else
      {
        // move file to reprocess dir
        file_move_back_to_input_dir_(file_name_info, file->full_path().c_str());
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;

      // copy the erroneous file to the error folder
      try
      {
        AdServer::LogProcessing::FileStore file_store(
          DIR_, DEFAULT_ERROR_DIR);
        file_store.store(file->full_path());
      }
      catch (const eh::Exception& store_ex)
      {
        ostr << FUN << store_ex.what() << " Can't copy the file '" <<
          file->full_path() << "' to the error folder. Initial error: " <<
          ex.what() << std::endl;
      }

      ostr << ex.what();
      callback_->report_error(
        Generics::ActiveObjectCallback::ERROR, ostr.str());
    }
  }

  void
  UserBindOperationFetcher::file_move_back_to_input_dir_(
    const AdServer::LogProcessing::LogFileNameInfo& info,
    const char* file_path) /*throw(eh::Exception)*/
  {
    static const char* FUN = "UserBindOperationFetcher::file_move_back_to_input_dir_()";

    const std::string new_file_name =
      AdServer::LogProcessing::restore_log_file_name(info, DIR_);
    
    std::string file_name;
    AdServer::PathManip::split_path(new_file_name.c_str(), 0, &file_name);
    std::string reprocess_path = unprocessed_dir_;
    reprocess_path += "/";
    reprocess_path += file_name;
    if(::rename(file_path, reprocess_path.c_str()) != 0)
    {
      Stream::Error ostr;
      ostr << FUN << "can't move file '" << file_path << "' to '" <<
        reprocess_path << "'";
      eh::throw_errno_exception<Exception>(ostr.str());
    }
  }

  void
  UserBindOperationFetcher::read_operation_(
    uint32_t operation_type,
    Generics::SmartMemBuf* mem_buf_ptr)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "UserBindOperationFetcher::read_operation_()";

    const Generics::MemBuf& mem_buf = mem_buf_ptr->membuf();
    if(mem_buf.size() < 4)
    {
      Stream::Error ostr;
      ostr << FUN << ": input buffer have incorrect size: " << mem_buf.size();
      throw Exception(ostr);
    }

    const void* data_buf = mem_buf.data();
    const unsigned long data_size = mem_buf.size();

    if(operation_type == UserBindOperationSaver::OP_ADD_USER_ID)
    {
      read_add_user_id_operation_(data_buf, data_size);
    }
    else if(operation_type == UserBindOperationSaver::OP_GET_USER_ID)
    {
      read_get_user_id_operation_(data_buf, data_size);
    }
    else
    {
      Stream::Error ostr;
      ostr << FUN << ": unknown operation: " << operation_type;
      throw Exception(ostr);
    }
  }

  void
  UserBindOperationFetcher::read_get_user_id_operation_(
    const void* data_buf,
    unsigned long data_size)
    /*throw(eh::Exception)*/
  {
    UserBindGetOperationReader reader(data_buf, data_size);

    user_bind_processor_->get_user_id(
      String::SubString(reader.external_id()),
      Commons::UserId(reader.current_user_id()),
      Generics::Time(reader.time()),
      false,
      Generics::Time::ZERO,
      false // for_set_cookie
      );
  }

  void
  UserBindOperationFetcher::read_add_user_id_operation_(
    const void* data_buf,
    unsigned long data_size)
    /*throw(eh::Exception)*/
  {
    UserBindAddOperationReader reader(data_buf, data_size);

    user_bind_processor_->add_user_id(
      String::SubString(reader.external_id()),
      Commons::UserId(reader.user_id()),
      Generics::Time(reader.time()),
      reader.resave_if_exists(),
      true // ignore_bad_event
      );
  }

  // UserBindOperationLoader
  UserBindOperationLoader::UserBindOperationLoader(
    Generics::ActiveObjectCallback* callback,
    UserBindProcessor* user_bind_operation_processor,
    const char* operation_file_in_dir,
    const char* unprocessed_dir,
    const char* file_prefix,
    const Generics::Time& check_period,
    std::size_t threads_count,
    const ChunkIdSet& chunk_ids)
    noexcept
  {
    Generics::ActiveObject_var interrupter = new InterrupterImpl();
    add_child_object(interrupter);

    AdServer::LogProcessing::FileProcessor_var operation_fetcher =
      new UserBindOperationFetcher(
        callback,
        user_bind_operation_processor,
        operation_file_in_dir,
        unprocessed_dir,
        file_prefix,
        chunk_ids,
        interrupter);

    AdServer::LogProcessing::FileThreadProcessor_var file_thread_processor =
      new AdServer::LogProcessing::FileThreadProcessor(
        operation_fetcher,
        callback,
        threads_count,
        (std::string(operation_file_in_dir) + "/Intermediate").c_str(),
        FETCH_FILES_LIMIT,
        operation_file_in_dir,
        file_prefix,
        check_period);

    add_child_object(file_thread_processor);
  }

  UserBindOperationLoader::~UserBindOperationLoader() noexcept
  {}

}
}
