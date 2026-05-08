#pragma once

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
