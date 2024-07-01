#include <libgen.h>

#include <Generics/Time.hpp>
#include <Generics/DirSelector.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/PathManip.hpp>

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
}

namespace AdServer
{
  struct MaxFileSelector
  {
    bool operator()(const char* full_path, const struct stat&) const
      /*throw(eh::Exception)*/
    {
      const char* name = Generics::DirSelect::file_name(full_path);        
      if(max_file_name < name)
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

  /* DirectoryModule::VersionedFileCacheConfiguration */
  DirectoryModule::VersionedFileCacheConfiguration::
  VersionedFileCacheConfiguration(const Generics::Time& check_period)
    noexcept
    : check_period_(check_period)
  {}

  bool
  DirectoryModule::VersionedFileCacheConfiguration::update_required(
    const Generics::StringHashAdapter& /*key*/,
    const Holder& holder)
    noexcept
  {
    bool req = holder.timestamp + check_period_ <
      Generics::Time::get_time_of_day();
    return req;
  }

  DirectoryModule::VersionedFileCacheConfiguration::Holder
  DirectoryModule::VersionedFileCacheConfiguration::update(
    const Generics::StringHashAdapter& key,
    const Holder* old_holder)
    /*throw(Exception)*/
  {
    static const char* FUN = "VersionedFileCacheConfiguration::update()";

    const std::string& file_folder = key.text();

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
        ostr << FUN << ": can't fetch directory '" << file_folder << "' content: " <<
          ex.what();
        throw Exception(ostr);
      }

      // if stat failed recheck directory content - next try
      // at last try open that found
      if(dir_select_success ||
         (try_count + 1 >= MAX_TRY_COUNT &&
           !max_file_selector.max_file_name.empty()))
      {
        const std::string& max_file_name = max_file_selector.max_file_name;

        if(old_holder &&
           max_file_name == old_holder->content->file_name())
        {
          Holder new_holder(*old_holder);
          new_holder.timestamp = Generics::Time::get_time_of_day();
          return new_holder;
        }

        std::string full_file_name = key.text() + "/" + max_file_name;

        // reopen file (don't keep file descriptor opened)
        int fd = ::open(full_file_name.c_str(), O_RDONLY, 0666);

        try
        {
          if(fd < 0)
          {
            int error = errno;
            if(error == ENOENT)
            {
              // next try - file already unlinked, must be present fresh file
            }
            else
            {
              eh::throw_errno_exception<Exception>(error, "");
            }
          }
          else
          {
            Generics::MMapFile mmapped_file(fd);

            Holder new_holder;

            new_holder.content = new FileContent(
              max_file_name.c_str(),
              mmapped_file.memory(),
              mmapped_file.length());

            new_holder.timestamp = Generics::Time::get_time_of_day();

            return new_holder;
          }
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't open file '" << max_file_name << "': " <<
            ex.what();
          throw Exception(ostr);
        }
      }

      ++try_count;
    }
    while(try_count < MAX_TRY_COUNT);

    if(old_holder)
    {
      return *old_holder;
    }

    // if all tries failed and no old holder
    Stream::Error ostr;
    ostr << FUN << ": can't open file '" << key.text() << "': all tries failed";
    throw Exception(ostr);
  }

  unsigned long
  DirectoryModule::VersionedFileCacheConfiguration::size(
    const Holder& holder) const
    noexcept
  {
    return holder.content->file_name().size() + holder.content->length() + 2;
  }

  DirectoryModule::FileContent_var
  DirectoryModule::VersionedFileCacheConfiguration::adapt(
    const DirectoryModule::VersionedFileCacheConfiguration::Holder& holder) const
    noexcept
  {
    return holder.content;
  }

  /* DirectoryModule */
  DirectoryModule::DirectoryModule(
    const GrpcContainerPtr& grpc_container,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
    Configuration* frontend_config,
    Logging::Logger* logger,
    FrontendCommons::HttpResponseFactory* response_factory)
    /*throw(eh::Exception)*/
    : FrontendCommons::FrontendInterface(response_factory),
      Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().ContentFeConfiguration()->Logger().log_level())),
        0,
        Aspect::DIRECTORY_MODULE,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        response_factory,
        frontend_config->get().ContentFeConfiguration()->threads(),
        0), // max pending tasks
      grpc_container_(grpc_container),
      task_processor_(task_processor),
      scheduler_(scheduler),
      frontend_config_(ReferenceCounting::add_ref(frontend_config))
  {}

  void DirectoryModule::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "AdServer::DirectoryModule::parse_configs_()";

    /* load common configuration */

    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if(!fe_config.ContentFeConfiguration().present())
      {
        throw Exception("ContentFeConfiguration isn't present");
      }

      config_.reset(
        new ContentFeConfiguration(*fe_config.ContentFeConfiguration()));

    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN  << "': " << e.what();
      throw Exception(ostr);
    }
  }


  bool
  DirectoryModule::will_handle(
    const String::SubString& uri) noexcept
  {
    static const char* FUN = "DirectoryModule::will_handle()";

    bool handle_it = false;

    if(!uri.empty())
    {
      if(!directories_.empty())
      {
        DirAliasMap::const_iterator it = directories_.upper_bound(uri.str());

        if(it != directories_.begin())
        {
          --it;

          if(it->first.compare(0, it->first.size(), uri.str(), 0, it->first.size()) == 0)
          {
            handle_it = true;
          }
        }
      }

      if(logger()->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": uri '" << uri << "':";
        for(DirAliasMap::const_iterator it = directories_.begin();
            it != directories_.end(); ++it)
        {
          ostr << " '" << it->first << "'";
        }
        ostr << ": " << handle_it;

        logger()->log(ostr.str(),
          Logging::Logger::TRACE,
          Aspect::DIRECTORY_MODULE);
      }
    }

    return handle_it;
  }

  void
  DirectoryModule::handle_request_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    noexcept
  {
    const FrontendCommons::HttpRequest& request = request_holder->request();

    FrontendCommons::HttpResponse_var response_ptr = create_response();
    FrontendCommons::HttpResponse& response = *response_ptr;
    int http_status = handle_request_(request, response);
    response_writer->write(http_status, response_ptr);
  }

  int
  DirectoryModule::handle_request_(
    const FrontendCommons::HttpRequest& request,
    FrontendCommons::HttpResponse& response)
    noexcept
  {
    static const char* FUN = "DirectoryModule::handle_request";

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
      if(params_pos != std::string::npos)
      {
        in_dir_path.resize(params_pos);
      }

      if(!AdServer::PathManip::normalize_path(in_dir_path))
      {
        return 403; // Forbidden
      }
      
      full_file_name = dir->root + in_dir_path;
    }

    std::string dir_name;
    std::string base_name;
    std::string::size_type dir_pos = full_file_name.rfind('/');
    if(dir_pos != std::string::npos)
    {
      dir_name.assign(full_file_name, 0, dir_pos);
      base_name = full_file_name.c_str() + dir_pos + 1;
    }
    else
    {
      base_name = full_file_name;
    }

    if(!base_name.empty())
    {
      FileContent_var file_content;

      try
      {
        file_content = dir->cache->get(dir_name);
      }
      catch(const eh::Exception& ex)
      {
        if(logger()->log_level() >= Logging::Logger::TRACE)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't open '" << dir_name << "' -> '" <<
            base_name << "' by '" << full_file_name << "': " <<
            ex.what();
          logger()->log(ostr.str(),
            Logging::Logger::TRACE,
            Aspect::DIRECTORY_MODULE);
        }

        return 404; // Not found
      }

      try
      {
        if(file_content->file_name() >= base_name)
        {
          response.add_header(
            Header::CACHE_CONTROL,
            DirectoryConfig::DEFAULT_REQUESTED_FRESH_OR_OLDEST_VERSION_CACHE_HEADER);
        }
        else
        {
          response.add_header(
            Header::CACHE_CONTROL,
            DirectoryConfig::DEFAULT_REQUEST_GREAT_VERSION_CACHE_HEADER);
        }

        response.set_content_type(DirectoryConfig::DEFAULT_CONTENT_TYPE_HEADER);

        response.get_output_stream().write(
          static_cast<const char*>(file_content->data()),
          file_content->length());
      }
      catch(const eh::Exception& ex)
      {
        return 0;
      }
    }
    else
    {
      if(logger()->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": can determine dirname, basename for '" <<
          full_file_name << "'";
        logger()->log(ostr.str(),
          Logging::Logger::TRACE,
          Aspect::DIRECTORY_MODULE);
      }

      return 404; // Not found
    }

    return 0; //OK
  }

  void
  DirectoryModule::init() /*throw(eh::Exception)*/
  {
    logger()->log(String::SubString(
        "DirectoryModule::init: frontend is running ..."),
      Logging::Logger::INFO, Aspect::DIRECTORY_MODULE);

    parse_configs_();

    for(auto it = config_->DirectoryList().Directory().begin();
        it != config_->DirectoryList().Directory().end(); ++it)
    {
       Directory_var new_directory(new Directory);
       new_directory->root = it->root();
       new_directory->cache = new VersionedFileCache(
         10*1024*1024,
         Generics::Time(1),
         VersionedFileCacheConfiguration(Generics::Time(2)));
       directories_[it->path()] = new_directory;
    }

    activate_object();
  }

  void
  DirectoryModule::shutdown() noexcept
  {
    logger()->log(String::SubString(
        "DirectoryModule::shutdown: frontend terminated"),
      Logging::Logger::INFO, Aspect::DIRECTORY_MODULE);
  }
}
