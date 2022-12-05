#ifndef _FILE_SYSTEM_H_
#define _FILE_SYSTEM_H_

#include <cstdio>
#include <fcntl.h>
#include <ftw.h>

namespace fs
{

  class FileSystem
  {
  public:

    void
    rename(
      const std::string& old_full_name, const std::string& new_full_name)
      /*throw(std::exception)*/;

    void
    create(const std::string& path, const std::string& file_name)
      /*throw(std::exception)*/;

    bool
    remove(const std::string& full_path) /*throw(std::exception)*/;

    void
    mkdir(const std::string& path) /*throw(std::exception)*/;

    void
    rmrf(const std::string& path) /*throw(std::exception)*/;

    char*
    getcwd(char* buf, size_t size) noexcept;
  };

  FileSystem file_system;

}

namespace fs
{
  char*
  FileSystem::getcwd(char* buf, size_t size) noexcept
  {
    return ::getcwd(buf, size);
  }

  void
  FileSystem::rename(
    const std::string& old_full_name, const std::string& new_full_name)
    /*throw(std::exception)*/
  {
    std::rename(old_full_name.c_str(), new_full_name.c_str());
  }

  void
  FileSystem::create(
    const std::string& path, const std::string& file_name)
    /*throw(std::exception)*/
  {
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    const std::string full_name = path + "/" + file_name;
    ::close(::creat(full_name.c_str(), mode));
  }

  bool
  FileSystem::remove(const std::string& full_path)
    /*throw(std::exception)*/
  {
    return (::unlink(full_path.c_str()) == 0);
  }

  void
  FileSystem::FileSystem::mkdir(const std::string& path)
    /*throw(std::exception)*/
  {
    ::mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP);
  }

  int unlink_cb(const char *fpath, const struct stat*, int, struct FTW*)
  {
      int rv = remove(fpath);

      if (rv)
          perror(fpath);

      return rv;
  }

  void
  FileSystem::rmrf(const std::string& path)
    /*throw(std::exception)*/
  {
    ::nftw(path.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
  }
}

#endif // _FILE_SYSTEM_H_
