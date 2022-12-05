#ifndef XSLTFILECACHE_HPP
#define XSLTFILECACHE_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <String/TextTemplate.hpp>
#include <Generics/FileCache.hpp>
#include <Commons/Xslt/XslTransformer.hpp>

namespace AdServer
{
  class XsltTextTemplateArgs:
    public ReferenceCounting::AtomicImpl,
    public String::TextTemplate::Args
  {
  protected:
    virtual ~XsltTextTemplateArgs() noexcept {}
  };

  typedef
    ReferenceCounting::QualPtr<XsltTextTemplateArgs>
    XsltTextTemplateArgs_var;

  typedef
    ReferenceCounting::ConstPtr<XsltTextTemplateArgs>
    CXsltTextTemplateArgs_var;

  class XsltFileUpdateStrategy
  {
  private:
    typedef std::unique_ptr<XslTransformer> XslTransformerPtr;

  public:
    struct Buffer
    {
      XslTransformerPtr transformer;
      Generics::Time timestamp;
    };

    XsltFileUpdateStrategy(
      const char* /*file*/) /*throw(eh::Exception)*/
    {
      assert(0);
    }

    XsltFileUpdateStrategy(
      const XsltTextTemplateArgs* args,
      const char* file) /*throw(eh::Exception)*/;

    Buffer& get() noexcept;

    void update()
      /*throw(Generics::CacheExceptions::CacheException, eh::Exception)*/;

  private:
    const std::string file_name_;
    CXsltTextTemplateArgs_var args_;
    Buffer xsl_transformer_;
  };

  typedef Generics::Cache<
    Generics::SimpleFileCheckStrategy,
    XsltFileUpdateStrategy>
    XsltFileCache;

  typedef XsltFileCache::Cache_var XsltFileCache_var;

  // XsltFileCacheFactory
  class XsltFileCacheFactory
  {
  public:
    XsltFileCacheFactory(const XsltTextTemplateArgs* args = 0);

    XsltFileCacheFactory(const XsltFileCacheFactory& init);

    virtual ~XsltFileCacheFactory() noexcept {};

    XsltFileCache_var
    operator()(const char* file_name) /*throw(eh::Exception)*/;

  private:
    CXsltTextTemplateArgs_var args_;
  };

  // XsltFileCacheManager
  class XsltFileCacheManager:
    public ReferenceCounting::AtomicImpl,
    public Generics::CacheManager<
      XsltFileCache,
      Generics::DefaultSizePolicy<std::string, XsltFileCache::Cache_var>,
      XsltFileCacheFactory>
  {
  public:
    XsltFileCacheManager(
      const XsltTextTemplateArgs* args,
      const Generics::Time& threshold_timeout,
      size_t bound_limit)
      /*throw(eh::Exception)*/;

  protected:
    virtual ~XsltFileCacheManager() noexcept {}
  };

  typedef ReferenceCounting::QualPtr<XsltFileCacheManager>
    XsltFileCacheManager_var;
}

#endif /*XSLTFILECACHE_HPP*/
