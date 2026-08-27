#pragma once

#include <cstring>
#include <memory>
#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Generics/ArrayAutoPtr.hpp>
#include <Generics/MMap.hpp>
#include <Generics/TaskRunner.hpp>
#include <HTTP/Http.hpp>

#include <Commons/AsyncCache.hpp>
#include <Commons/ExecutorPool.hpp>
#include <Frontends/FrontendCommons/HTTPExceptions.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>

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
    public virtual FrontendCommons::CoroFrontendInterface,
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
    using Exception = FrontendCommons::HTTPExceptions::Exception;
  public:
    DirectoryModule(
      Configuration* frontend_config_,
      Logging::Logger* logger,
      std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    FrontendCommons::RequestTask
    co_handle_request(FCGI::HttpRequestHolder_var request_holder)
      noexcept override;

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
    class FileContent
    {
    public:
      FileContent(const char* file_name, const void* buf, unsigned long buf_len)
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

      ~FileContent() noexcept = default;

    private:
      const std::string file_name_;
      const Generics::ArrayAutoPtr<char> buf_;
      const unsigned long length_;
    };

    using FileContentPtr = std::shared_ptr<FileContent>;

    DECLARE_EXCEPTION(VersionedFileCacheException, eh::DescriptiveException);

    using VersionedFileCache = AdServer::Commons::AsyncCache<std::string, FileContentPtr>;
    using VersionedFileCachePtr = std::shared_ptr<VersionedFileCache>;

    struct Directory: public ReferenceCounting::AtomicImpl
    {
      std::string root;
      VersionedFileCachePtr cache;

    protected:
      virtual ~Directory() noexcept {}
    };

    using Directory_var = ReferenceCounting::SmartPtr<Directory>;

    using DirAliasMap = std::map<std::string, Directory_var>;

    using ContentFeConfiguration = Configuration::FeConfig::ContentFeConfiguration_type;

    using ConfigPtr = std::unique_ptr<ContentFeConfiguration>;

  private:
    void
    parse_configs_() /*throw(Exception)*/;

    int
    fill_response_(
      FCGI::HttpResponse& response,
      const std::string& base_name,
      const FileContentPtr& file_content)
      noexcept;

    FrontendCommons::RequestTask
    process_request_(FCGI::HttpRequestHolder_var request_holder, FCGI::HttpResponse_var response)
      noexcept;

    static VersionedFileCache::HolderPtr
    load_versioned_file_(
      const std::string& file_folder,
      const VersionedFileCache::HolderPtr& old_holder);

    static VersionedFileCache::SyncUpdate
    make_versioned_file_sync_update_();

    static VersionedFileCache::AsyncUpdate
    make_versioned_file_async_update_(Generics::TaskRunner* task_runner);

  private:
    Generics::TaskRunner_var versioned_file_task_runner_;
    DirAliasMap directories_;
    ConfigPtr config_;
    Configuration_var frontend_config_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers_;
  };
}
