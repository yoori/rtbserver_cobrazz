#ifndef MMAPFILECACHE_HPP
#define MMAPFILECACHE_HPP

namespace AdServer
{
  struct File: public Generics::MMapFile,
    public ReferenceCounting::AtomicImpl
  {
    File(const char* filename) noexcept;

  private:
    virtual ~File() noexcept {};
  };

  typedef Generics::BoundedCache<
    Generics::StringHashAdapter,
    File_var,
    TextTemplateCacheConfiguration,
    Generics::GnuHashTable>
    TextTemplateCache;
}

#endif /*MMAPFILECACHE_HPP*/
