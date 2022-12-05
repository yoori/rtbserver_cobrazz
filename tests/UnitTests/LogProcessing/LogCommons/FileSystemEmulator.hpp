#ifndef _FILE_SYSTEM_EMULATOR_H_
#define _FILE_SYSTEM_EMULATOR_H_

#include <set>
#include <string>
#include <iostream>

#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

namespace fs
{

  typedef Sync::Policy::PosixThread SyncPolicy;

  class Dir
    : public std::set<std::string>, public ReferenceCounting::AtomicImpl
  {
  public:
    std::string name;
    mutable SyncPolicy::Mutex mutex;

  public:
    virtual
    ~Dir() noexcept
    {}
  };
  
  typedef ReferenceCounting::SmartPtr<Dir> Dir_var;

  class Dir_var_less
    : public std::binary_function<bool, Dir_var, Dir_var>
  {
  public:
    bool
    operator() (const Dir_var& dir1, const Dir_var& dir2) const
    {
      return (dir1->name < dir2->name);
    }
  };

  class FileSystem: public std::set<Dir_var, Dir_var_less>
  {
  public:
    FileSystem::iterator
    find(const std::string& name) noexcept;

    void
    rename(
      const std::string& old_full_name, const std::string& new_full_name)
      /*throw(std::exception)*/;

    void
    create(const std::string& path, const std::string& file_name)
      /*throw(std::exception)*/;

    bool
    remove(const std::string& full_path) /*throw(std::exception)*/;

    bool
    access(const std::string& full_path) /*throw(std::exception)*/;

    void
    mkdir(const std::string& path) /*throw(std::exception)*/;

    void
    rmrf(const std::string& path) /*throw(std::exception)*/;

    char*
    getcwd(char* buf, size_t size) noexcept;
  };

  FileSystem file_system;

  namespace
  {

    void
    split_to_path_and_file_name(
      const std::string& full_path,
      std::string& path, std::string& file_name) /*throw(std::exception)*/
    {
      std::string::size_type pos = full_path.rfind("/");
      path = full_path.substr(0, pos);
      file_name = full_path.substr(pos + 1);
    }

  }

  FileSystem::iterator
  FileSystem::find(const std::string& name) noexcept
  {
    Dir_var tmp(new Dir());
    tmp->name = name;
    return std::set<Dir_var, Dir_var_less>::find(tmp);
  }

  char*
  FileSystem::getcwd(char* buf, size_t) noexcept
  {
    buf[0] = '.';
    buf[1] = 0;
    return buf;
  }

  void
  FileSystem::rename(
    const std::string& old_full_name, const std::string& new_full_name)
    /*throw(std::exception)*/
  {
    std::string old_path, old_file_name;
    split_to_path_and_file_name(old_full_name, old_path, old_file_name);

    std::string new_path, new_file_name;
    split_to_path_and_file_name(new_full_name, new_path, new_file_name);

    FileSystem::iterator oi = file_system.find(old_path);
    FileSystem::iterator ni = file_system.find(new_path);

    if (oi != file_system.end() && ni != file_system.end())
    {
      bool exist = false;
      
      {
        SyncPolicy::WriteGuard lock((*oi)->mutex);
        exist = ((*oi)->find(old_file_name) != (*oi)->end());
      }

      if (exist)
      {
        {
          Dir& old_dir = const_cast<Dir&>(**oi);
          SyncPolicy::WriteGuard lock(old_dir.mutex);
          old_dir.erase(old_file_name);
        }

        {
          Dir& new_dir = const_cast<Dir&>(**ni);
	  SyncPolicy::WriteGuard lock(new_dir.mutex);
          new_dir.insert(new_file_name);
	}
      }
    }
  }

  void
  FileSystem::create(
    const std::string& path, const std::string& file_name)
    /*throw(std::exception)*/
  {
    FileSystem::iterator di = file_system.find(path);

    if (di == file_system.end())
    {
      Dir_var dir(new Dir());
      dir->name = path;
      di = file_system.insert(dir).first;
    }

    Dir::iterator fi = (*di)->find(file_name);

    if (fi == (*di)->end())
    {
      Dir& dir = const_cast<Dir&>(**di);
      SyncPolicy::WriteGuard lock(dir.mutex);
      dir.insert(file_name);
    }
  }

  bool
  FileSystem::remove(const std::string& full_path)
    /*throw(std::exception)*/
  {
    std::string path, file_name;
    split_to_path_and_file_name(full_path, path, file_name);

    FileSystem::iterator di = file_system.find(path);

    if (di != file_system.end())
    {
      Dir& dir = const_cast<Dir&>(**di);
      
      SyncPolicy::WriteGuard lock(dir.mutex);
      Dir::iterator fi = dir.find(file_name);

      if (fi != dir.end())
      {
        dir.erase(file_name);
        return true;
      }
    }

    return false;
  }

  bool
  FileSystem::access(const std::string& full_path) /*throw(std::exception)*/
  {
    std::string path, file_name;
    split_to_path_and_file_name(full_path, path, file_name);

    FileSystem::iterator di = file_system.find(path);

    if (di != file_system.end())
    {
      Dir& dir = const_cast<Dir&>(**di);
      return (dir.find(file_name) != dir.end());
    }

    return false;
  }

  void
  FileSystem::mkdir(const std::string& path)
    /*throw(std::exception)*/
  {
    Dir_var dir(new Dir());
    dir->name = path;
    file_system.insert(dir);
  }
  
  void
  FileSystem::rmrf(const std::string& path)
    /*throw(std::exception)*/
  {
    FileSystem::iterator di = file_system.find(path);
    erase(di);
  }

}

#endif //_FILE_SYSTEM_EMULATOR_H_
