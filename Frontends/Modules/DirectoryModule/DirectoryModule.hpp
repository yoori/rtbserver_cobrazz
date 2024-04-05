#ifndef ADSERVER_FRONTENDS_DIRECTORYMODULE_HPP
#define ADSERVER_FRONTENDS_DIRECTORYMODULE_HPP

#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Generics/MMap.hpp>
#include <HTTP/Http.hpp>
#include <UServerUtils/Grpc/Core/Common/Scheduler.hpp>
#include <userver/engine/task/task_processor.hpp>

#include <FrontendCommons/BoundedCache.hpp>
#include <Frontends/FrontendCommons/HTTPExceptions.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>

namespace AdServer
{
  
  /*
   *   VersioningDirectory
   **
   *   path/file -> path/<file without version suffix>.<greatest version>
   */
  class DirectoryModule:
    private Logging::LoggerCallbackHolder,
    private FrontendCommons::HTTPExceptions,
    public FrontendCommons::FrontendTaskPool,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    using TaskProcessor = userver::engine::TaskProcessor;
    using SchedulerPtr = UServerUtils::Grpc::Core::Common::SchedulerPtr;
    using Exception = FrontendCommons::HTTPExceptions::Exception;

  public:
    DirectoryModule(
      TaskProcessor& task_processor,
      const SchedulerPtr& scheduler,
      Configuration* frontend_config_,
      Logging::Logger* logger,
      FrontendCommons::HttpResponseFactory* response_factory)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    virtual void
    handle_request_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
      noexcept;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

  protected:
    virtual
    ~DirectoryModule() noexcept = default;

  private:
    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    /* versioned files cache
     * requirements: file content can't be changed after creation
     */
    class FileContent: public ReferenceCounting::AtomicImpl
    {
    public:
      FileContent(
        const char* file_name,
        const void* buf,
        unsigned long buf_len)
        /*throw(eh::Exception)*/
        : file_name_(file_name),
          buf_(buf_len),
          length_(buf_len)
      {
        ::memcpy(buf_.get(), buf, buf_len);
      }

      const std::string& file_name() const noexcept
      {
        return file_name_;
      }

      const void* data() const noexcept
      {
        return buf_.get();
      }

      unsigned long length() const noexcept
      {
        return length_;
      }

    protected:
      virtual ~FileContent() noexcept {}

    private:
      const std::string file_name_;
      const Generics::ArrayAutoPtr<char> buf_;
      const unsigned long length_;
    };

    typedef ReferenceCounting::SmartPtr<FileContent>
      FileContent_var;

    class VersionedFileCacheConfiguration
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct Holder
      {
        Generics::Time timestamp;
        FileContent_var content;
      };

    public:
      VersionedFileCacheConfiguration(const Generics::Time& check_period =
        Generics::Time(10)) noexcept;

      bool update_required(
        const Generics::StringHashAdapter& key,
        const Holder& file_state) noexcept;

      Holder update(
        const Generics::StringHashAdapter& key,
        const Holder* old_holder)
        /*throw(Exception)*/;

      unsigned long size(const Holder& file_state) const noexcept;

      FileContent_var adapt(const Holder& file_state) const noexcept;

    private:
      Generics::Time check_period_;
    };

    typedef BoundedCache<
      Generics::StringHashAdapter,
      FileContent_var,
      VersionedFileCacheConfiguration,
      AdServer::Commons::RCHash2Args>
      VersionedFileCache;

    typedef ReferenceCounting::SmartPtr<VersionedFileCache>
      VersionedFileCache_var;

    struct Directory: public ReferenceCounting::AtomicImpl
    {
      std::string root;
      VersionedFileCache_var cache;

    protected:
      virtual ~Directory() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<Directory> Directory_var;

    typedef std::map<std::string, Directory_var> DirAliasMap;

    typedef Configuration::FeConfig::ContentFeConfiguration_type
      ContentFeConfiguration;

    typedef std::unique_ptr<ContentFeConfiguration> ConfigPtr;

  private:
    void
    parse_configs_() /*throw(Exception)*/;

    int
    handle_request_(
      const FrontendCommons::HttpRequest& request,
      FrontendCommons::HttpResponse& response)
      noexcept;

  private:
    TaskProcessor& task_processor_;
    const SchedulerPtr scheduler_;
    DirAliasMap directories_;
    ConfigPtr config_;
    Configuration_var frontend_config_;
  };
}

#endif /*ADSERVER_FRONTENDS_DIRECTORYMODULE_HPP*/
