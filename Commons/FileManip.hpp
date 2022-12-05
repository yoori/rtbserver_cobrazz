#ifndef COMMONS_FILEMANIP_HPP_
#define COMMONS_FILEMANIP_HPP_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <eh/Exception.hpp>
#include <eh/Errno.hpp>
#include <String/SubString.hpp>

namespace AdServer
{
  namespace FileManip
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    bool
    dir_exists(const String::SubString& path) noexcept;

    bool
    file_exists(const String::SubString& path) noexcept;

    void
    rename(const String::SubString& src,
      const String::SubString& dst,
      bool ignore_non_existing)
      /*throw(eh::Exception)*/;
  }
}

namespace AdServer
{
namespace FileManip
{
  inline bool
  dir_exists(const String::SubString& path) noexcept
  {
    struct stat info;

    if(::stat(path.str().c_str(), &info) != 0)
    {
      return false;
    }
    else if(info.st_mode & S_IFDIR)
    {
      return true;
    }

    return false;
  }

  inline bool
  file_exists(const String::SubString& path) noexcept
  {
    struct stat info;

    if(::stat(path.str().c_str(), &info) != 0)
    {
      return false;
    }
    else if(info.st_mode & S_IFREG)
    {
      return true;
    }

    return false;
  }

  void
  rename(const String::SubString& src,
    const String::SubString& dst,
    bool ignore_non_existing)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "rename()";

    if(std::rename(src.str().c_str(), dst.str().c_str()))
    {
      if(!ignore_non_existing || errno != ENOENT)
      {
        eh::throw_errno_exception<Exception>(
          FUN,
          ": failed to rename file '",
          src.str().c_str(),
          "' to '",
          dst.str().c_str(),
          "'");
      }
    }
  }
}
}


#endif /*COMMONS_FILEMANIP_HPP_*/
