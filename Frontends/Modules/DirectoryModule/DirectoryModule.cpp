#include <libgen.h>

#include <Generics/Time.hpp>
#include <Generics/DirSelector.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/PathManip.hpp>
#include <Commons/DelegateTaskGoal.hpp>

#include "DirectoryModule.hpp"

namespace
{
  namespace Aspect
  {
    const char DIRECTORY_MODULE[] = "DirectoryModule";
  }

  namespace DirectoryConfig
  {
    const String::SubString DEFAULT_REQUESTED_FRESH_OR_OLDEST_VERSION_CACHE_HEADER(
      "max-age=2592000"); // 30 days
    const String::SubString DEFAULT_REQUEST_GREAT_VERSION_CACHE_HEADER(
      "max-age=10"); // 10 sec
    const String::SubString DEFAULT_CONTENT_TYPE_HEADER("text/html");
  }

  namespace Header
  {
    const String::SubString CACHE_CONTROL("Cache-Control");
    const String::SubString CONTENT_TYPE("Content-Type");
  }

  struct Constraints
  {
    static const unsigned long MAX_NUMBER_PARAMS = 50;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 30;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 2000;
  };

  const unsigned long MAX_TRY_COUNT = 3;
  const unsigned VERSIONED_FILE_LOAD_THREADS = 2;
  const std::size_t VERSIONED_FILE_CACHE_SIZE = 10 * 1024 * 1024;
  const Generics::Time VERSIONED_FILE_CACHE_UPDATE_PERIOD(2);
  const Generics::Time VERSIONED_FILE_CACHE_MAX_KEEP_TIME(30 * 24 * 60 * 60);
}

namespace AdServer
{
  struct MaxFileSelector
  {
    bool operator()(const char* full_path, const struct stat&) const
      /*throw(eh::Exception)*/
    {
      const char* name = Generics::DirSelect::file_name(full_path);
      if (max_file_name < name)
      {
        max_file_name = name;
      }

      return true;
    }

    mutable std::string max_file_name;
  };

  struct FailedFileStat
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static void throw_exception(const char*) /*throw(Exception)*/
    {
      throw Exception("");
    }
  };

  DirectoryModule::VersionedFileCache::HolderPtr
  DirectoryModule::load_versioned_file_(
    const std::string& file_folder,
    const VersionedFileCache::HolderPtr& old_holder)
  {
    static const char* FUN = "DirectoryModule::load_versioned_file_()";

    const FileContentPtr old_content =
      old_holder && old_holder->value ?
        *old_holder->value :
        FileContentPtr();

    unsigned long try_count = 0;

    MaxFileSelector max_file_selector;

    do
    {
      bool dir_select_success = true;

      // ls file versions
      try
      {
        Generics::DirSelect::directory_selector(
          file_folder.c_str(),
          max_file_selector,
          false,
          Generics::DirSelect::default_failed_to_open_directory,
          FailedFileStat::throw_exception);
      }
      catch(const FailedFileStat::Exception&)
      {
        dir_select_success = false;
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't fetch directory '" << file_folder << "' content: " << ex.what();
        throw VersionedFileCacheException(ostr);
      }

      // if stat failed recheck directory content - next try
      // at last try open that found
      if (dir_select_success ||
         (try_count + 1 >= MAX_TRY_COUNT && !max_file_selector.max_file_name.empty()))
      {
        const std::string& max_file_name = max_file_selector.max_file_name;

        if (old_content && max_file_name == old_content->file_name())
        {
          return std::make_shared<VersionedFileCache::Holder>(
            old_content,
            Generics::Time::get_time_of_day(),
            Generics::Time::ZERO,
            old_content->file_name().size() + old_content->length() + 2);
        }

        std::string full_file_name = file_folder + "/" + max_file_name;

        // reopen file (don't keep file descriptor opened)
        int fd = ::open(full_file_name.c_str(), O_RDONLY, 0666);

        try
        {
          if (fd < 0)
          {
            int error = errno;
            if (error == ENOENT)
            {
              // next try - file already unlinked, must be present fresh file
            }
            else
            {
              eh::throw_errno_exception<VersionedFileCacheException>(error, "");
            }
          }
          else
          {
            Generics::MMapFile mmapped_file(fd);

            FileContentPtr content = std::make_shared<FileContent>(
              max_file_name.c_str(),
              mmapped_file.memory(),
              mmapped_file.length());

            return std::make_shared<VersionedFileCache::Holder>(
              content,
              Generics::Time::get_time_of_day(),
              Generics::Time::ZERO,
              content->file_name().size() + content->length() + 2);
          }
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't open file '" << max_file_name << "': " << ex.what();
          throw VersionedFileCacheException(ostr);
        }
      }

      ++try_count;
    }
    while (try_count < MAX_TRY_COUNT);

    if (old_content)
    {
      return old_holder;
    }

    // if all tries failed and no old holder
    Stream::Error ostr;
    ostr << FUN << ": can't open file '" << file_folder << "': all tries failed";
    throw VersionedFileCacheException(ostr);
  }

  DirectoryModule::VersionedFileCache::SyncUpdate
  DirectoryModule::make_versioned_file_sync_update_()
  {
    return [](const std::string& file_folder, const VersionedFileCache::HolderPtr& old_holder) ->
        VersionedFileCache::HolderPtr
    {
      return load_versioned_file_(file_folder, old_holder);
    };
  }

  DirectoryModule::VersionedFileCache::AsyncUpdate
  DirectoryModule::make_versioned_file_async_update_(Generics::TaskRunner* task_runner)
  {
    return [task_runner](
      std::string file_folder,
      VersionedFileCache::HolderPtr old_holder,
      VersionedFileCache::UpdateCallback callback)
    {
      try
      {
        task_runner->enqueue_task(AdServer::Commons::make_delegate_task(
          [
            file_folder = std::move(file_folder),
            old_holder = std::move(old_holder),
            callback = std::move(callback)
          ]() mutable noexcept
          {
            VersionedFileCache::HolderPtr holder;
            try
            {
              holder = load_versioned_file_(file_folder, old_holder);
            }
            catch(...)
            {}

            try
            {
              callback(std::move(holder));
            }
            catch(...)
            {}
          }));
      }
      catch(...)
      {
        try
        {
          callback(VersionedFileCache::HolderPtr());
        }
        catch(...)
        {}
      }
    };
  }

  /* DirectoryModule */
  DirectoryModule::DirectoryModule(
    Configuration* frontend_config,
    Logging::Logger* logger,
    std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers)
    /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().ContentFeConfiguration()->Logger().log_level())),
        0,
        Aspect::DIRECTORY_MODULE,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      workers_(std::move(request_workers))
  {}

  void DirectoryModule::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "AdServer::DirectoryModule::parse_configs_()";

    /* load common configuration */

    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if (!fe_config.ContentFeConfiguration().present())
      {
        throw Exception("ContentFeConfiguration isn't present");
      }

      config_.reset(new ContentFeConfiguration(*fe_config.ContentFeConfiguration()));

    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN  << "': " << e.what();
      throw Exception(ostr);
    }
  }


  bool
  DirectoryModule::will_handle(const String::SubString& uri) noexcept
  {
    static const char* FUN = "DirectoryModule::will_handle()";

    bool handle_it = false;

    if (!uri.empty())
    {
      if (!directories_.empty())
      {
        DirAliasMap::const_iterator it = directories_.upper_bound(uri.str());

        if (it != directories_.begin())
        {
          --it;

          if (it->first.compare(0, it->first.size(), uri.str(), 0, it->first.size()) == 0)
          {
            handle_it = true;
          }
        }
      }

      if (logger()->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": uri '" << uri << "':";
        for (DirAliasMap::const_iterator it = directories_.begin(); it != directories_.end(); ++it)
        {
          ostr << " '" << it->first << "'";
        }
        ostr << ": " << handle_it;

        logger()->log(ostr.str(), Logging::Logger::TRACE, Aspect::DIRECTORY_MODULE);
      }
    }

    return handle_it;
  }

  FrontendCommons::RequestTask
  DirectoryModule::co_handle_request(FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    co_await AdServer::Commons::ExecutorPool::yield(workers_);

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    auto result = co_await process_request_(std::move(request_holder), std::move(response_ptr));
    co_return std::move(result);
  }

  int
  DirectoryModule::fill_response_(
    FCGI::HttpResponse& response,
    const std::string& base_name,
    const FileContentPtr& file_content)
    noexcept
  {
    try
    {
      if (file_content->file_name() >= base_name)
      {
        response.add_header_nocopy(
          Header::CACHE_CONTROL,
          DirectoryConfig::DEFAULT_REQUESTED_FRESH_OR_OLDEST_VERSION_CACHE_HEADER);
      }
      else
      {
        response.add_header_nocopy(
          Header::CACHE_CONTROL,
          DirectoryConfig::DEFAULT_REQUEST_GREAT_VERSION_CACHE_HEADER);
      }

      response.set_content_type_nocopy(DirectoryConfig::DEFAULT_CONTENT_TYPE_HEADER);

      response.get_output_stream().write(
        static_cast<const char*>(file_content->data()),
        file_content->length());
    }
    catch(const eh::Exception&)
    {
      return 0;
    }

    return 0; //OK
  }

  FrontendCommons::RequestTask
  DirectoryModule::process_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponse_var response)
    noexcept
  {
    static const char* FUN = "DirectoryModule::handle_request";

    const FCGI::HttpRequest& request = request_holder->request();

    Directory_var dir;
    std::string full_file_name;

    {
      DirAliasMap::const_iterator it = directories_.upper_bound(request.uri().str());
      assert(it != directories_.begin());
      --it;
      const std::string& dir_alias = it->first;
      dir = it->second;

      std::string in_dir_path(request.uri().str().c_str() + dir_alias.size());
      std::string::size_type params_pos = in_dir_path.find('?');
      if (params_pos != std::string::npos)
      {
        in_dir_path.resize(params_pos);
      }

      if (!AdServer::PathManip::normalize_path(in_dir_path))
      {
        co_return FrontendCommons::RequestResult{
          403,
          response,
          false};
      }

      full_file_name = dir->root + in_dir_path;
    }

    std::string dir_name;
    std::string base_name;
    std::string::size_type dir_pos = full_file_name.rfind('/');
    if (dir_pos != std::string::npos)
    {
      dir_name.assign(full_file_name, 0, dir_pos);
      base_name = full_file_name.c_str() + dir_pos + 1;
    }
    else
    {
      base_name = full_file_name;
    }

    if (!base_name.empty())
    {
      FileContentPtr file_content = co_await dir->cache->co_get(workers_, dir_name);
      if (!file_content)
      {
        if (logger()->log_level() >= Logging::Logger::TRACE)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't open '" << dir_name << "' -> '" <<
            base_name << "' by '" << full_file_name << "'";
          logger()->log(ostr.str(), Logging::Logger::TRACE, Aspect::DIRECTORY_MODULE);
        }

        co_return FrontendCommons::RequestResult{
          404,
          response,
          false};
      }

      const int http_status = fill_response_(*response, base_name, file_content);
      co_return FrontendCommons::RequestResult{
        http_status,
        response,
        false};
    }
    else
    {
      if (logger()->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": can determine dirname, basename for '" << full_file_name << "'";
        logger()->log(ostr.str(), Logging::Logger::TRACE, Aspect::DIRECTORY_MODULE);
      }

      co_return FrontendCommons::RequestResult{
        404,
        response,
        false};
    }
  }

  void
  DirectoryModule::init() /*throw(eh::Exception)*/
  {
    logger()->log(String::SubString("DirectoryModule::init: frontend is running ..."),
      Logging::Logger::INFO, Aspect::DIRECTORY_MODULE);

    parse_configs_();

    versioned_file_task_runner_ = new Generics::TaskRunner(callback(), VERSIONED_FILE_LOAD_THREADS);
    add_child_object(versioned_file_task_runner_);

    for (auto it = config_->DirectoryList().Directory().begin();
        it != config_->DirectoryList().Directory().end(); ++it)
    {
       Directory_var new_directory(new Directory);
       new_directory->root = it->root();
       new_directory->cache = std::make_shared<VersionedFileCache>(
         VERSIONED_FILE_CACHE_SIZE,
         VERSIONED_FILE_CACHE_UPDATE_PERIOD,
         VERSIONED_FILE_CACHE_MAX_KEEP_TIME,
         make_versioned_file_sync_update_(),
         make_versioned_file_async_update_(versioned_file_task_runner_));
       directories_[it->path()] = new_directory;
    }
  }

  void
  DirectoryModule::shutdown() noexcept
  {
    deactivate_object();
    wait_object();

    logger()->log(String::SubString("DirectoryModule::shutdown: frontend terminated"),
      Logging::Logger::INFO, Aspect::DIRECTORY_MODULE);
  }
}
