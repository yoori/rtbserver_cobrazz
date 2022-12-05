#ifndef TEXTTEMPLATECACHE_HPP
#define TEXTTEMPLATECACHE_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Time.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <String/SubString.hpp>
#include <String/TextTemplate.hpp>
#include <Commons/BoundedCache.hpp>
#include <Commons/CorbaTypes.hpp>

#include <sys/stat.h>
#include <fcntl.h>
#include <eh/Errno.hpp>
#include <Generics/MMap.hpp>

inline
void
operator <<(CORBACommons::OctSeq_out& corba_str, const std::string& str)
  /*throw(eh::Exception)*/
{
  CORBACommons::OctSeq::value_type* ptr = CORBACommons::OctSeq::allocation_traits::allocbuf(str.size());
  std::copy(str.begin(), str.end(), ptr);
  corba_str = new CORBACommons::OctSeq(str.size(), str.size(), ptr);
}

namespace AdServer
{
namespace Commons
{
  class TextTemplate:
    public String::TextTemplate::String,
    public ReferenceCounting::AtomicImpl
  {
  public:
    explicit
    TextTemplate(const ::String::SubString& str)
      /*throw(::String::TextTemplate::TextTemplException,
        eh::Exception)*/;

  private:
    virtual ~TextTemplate() noexcept {}
  };
  typedef ReferenceCounting::SmartPtr<TextTemplate>
    TextTemplate_var;

  class File:
    public std::string,
    public ReferenceCounting::AtomicImpl
  {
  public:
    explicit
    File(const ::String::SubString& str)
      /*throw(eh::Exception)*/
      : std::string(str.data(), str.size())
    {}

  private:
    virtual ~File() noexcept {}
  };
  typedef ReferenceCounting::SmartPtr<File>
    File_var;

  template <typename Text>
  struct TextTemplateCacheConfiguration
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef ReferenceCounting::SmartPtr<Text>
      Text_var;

    struct TextTemplateHolder:
      public ReferenceCounting::AtomicImpl
    {
      TextTemplateHolder(
        Text* templ_val,
        const Generics::Time& check_time_val,
        const Generics::Time& mod_time_val,
        unsigned long size_val)
        noexcept;

      const Text_var templ;
      const Generics::Time check_time;
      const Generics::Time mod_time;
      const unsigned long size;

    private:
      virtual ~TextTemplateHolder() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<TextTemplateHolder>
      TextTemplateHolder_var;
    typedef TextTemplateHolder_var Holder;

    class FarUpdater :
      public ReferenceCounting::DefaultImpl<>
    {
    public:
      virtual Holder
      far_update(const char* file, const char* /*service_id*/) /*throw(Exception)*/
      {
        eh::throw_errno_exception<Exception>(
          errno, "TextTemplateCacheConfiguration<Text>::update():"
            " failed to stat file '", file, "'");
        return Holder();
      }
    protected:
      virtual ~FarUpdater() noexcept {}
    };
    typedef ReferenceCounting::SmartPtr<FarUpdater>
      FarUpdater_var;

    explicit
    TextTemplateCacheConfiguration(const Generics::Time& check_period,
      FarUpdater* updater = nullptr)
      noexcept;

    bool
    update_required(
      const Generics::StringHashAdapter& path,
      const TextTemplateHolder* val)
      const
      noexcept;

    TextTemplateHolder_var
    update(
      const Generics::StringHashAdapter& path,
      const TextTemplateHolder_var* val,
      const char* service_id)
      const /*throw(Exception)*/;

    unsigned long
    size(const TextTemplateHolder* val) const noexcept;

    Text_var
    adapt(const TextTemplateHolder* val) const noexcept;

  private:
    const Generics::Time check_period_;
    FarUpdater_var updater_;
  };

  typedef BoundedCache<
    Generics::StringHashAdapter,
    TextTemplate_var,
    TextTemplateCacheConfiguration<TextTemplate>,
    AdServer::Commons::RCHash2Args>
    TextTemplateCache;

  typedef ReferenceCounting::SmartPtr<TextTemplateCache>
    TextTemplateCache_var;

  typedef BoundedCache<
    Generics::StringHashAdapter,
    File_var,
    TextTemplateCacheConfiguration<File>,
    AdServer::Commons::RCHash2Args>
    BoundedFileCache;

  typedef ReferenceCounting::SmartPtr<BoundedFileCache>
    BoundedFileCache_var;
}
}

namespace AdServer
{
namespace Commons
{
  inline
  TextTemplate::TextTemplate(const ::String::SubString& str)
    /*throw(::String::TextTemplate::TextTemplException,
      eh::Exception)*/
    : String(str, ::String::SubString("##"), ::String::SubString("##"))
  {}

  template <typename Text>
  inline
  TextTemplateCacheConfiguration<Text>::
  TextTemplateHolder::TextTemplateHolder(
    Text* templ_val,
    const Generics::Time& check_time_val,
    const Generics::Time& mod_time_val,
    unsigned long size_val)
    noexcept
    : templ(ReferenceCounting::add_ref(templ_val)),
      check_time(check_time_val),
      mod_time(mod_time_val),
      size(size_val)
  {}

  template <typename Text>
  inline
  TextTemplateCacheConfiguration<Text>::TextTemplateCacheConfiguration(
    const Generics::Time& check_period,
    FarUpdater* updater)
    noexcept
    : check_period_(check_period),
    updater_(updater ? updater : new FarUpdater)
  {}

  template <typename Text>
  inline
  bool
  TextTemplateCacheConfiguration<Text>::update_required(
    const Generics::StringHashAdapter& /*path*/,
    const TextTemplateHolder* val) const
    noexcept
  {
    return val->check_time + check_period_ <=
      Generics::Time::get_time_of_day();
  }

  template <typename Text>
  inline
  unsigned long
  TextTemplateCacheConfiguration<Text>::size(
    const TextTemplateHolder* val) const noexcept
  {
    return val->size;
  }

  template <typename Text>
  inline
  typename TextTemplateCacheConfiguration<Text>::Text_var
  TextTemplateCacheConfiguration<Text>::adapt(
    const TextTemplateHolder* val) const noexcept
  {
    return val->templ;
  }

  template <typename Text>
  inline
  typename TextTemplateCacheConfiguration<Text>::TextTemplateHolder_var
  TextTemplateCacheConfiguration<Text>::update(
    const Generics::StringHashAdapter& path,
    const TextTemplateHolder_var* val,
    const char* service_id) const
    /*throw(Exception)*/
  {
    static const char* FUN = "TextTemplateCacheConfiguration<Text>::update()";

    Generics::Time now = Generics::Time::get_time_of_day();

    struct stat fs;

    if(::stat(path.text().c_str(), &fs) < 0)
    {
      // Call CampaignManagersPool->get_file(filename)
      return updater_->far_update(path.text().data() + path.text().rfind('/') + 1, service_id);
    }

    Generics::Time new_mod_time(fs.st_mtime);

    if(!val || new_mod_time != (*val)->mod_time)
    {
      int fd = ::open(path.text().c_str(), O_RDONLY);

      if(fd < 0)
      {
        eh::throw_errno_exception<Exception>(
          errno, FUN, ": failed to open file '", path.text(), "'");
      }

      try
      {
        Generics::MMap mapped_file(fd);

        return new TextTemplateHolder(
          Text_var(new Text(String::SubString(
            static_cast<const char*>(mapped_file.memory()),
            mapped_file.length()))),
          now,
          new_mod_time,
          mapped_file.length());
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    return new TextTemplateHolder(
      (*val)->templ,
      now,
      new_mod_time,
      (*val)->size);
  }

}
}

#endif //TEXTTEMPLATECACHE_HPP
