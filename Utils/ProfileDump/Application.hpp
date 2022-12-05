#ifndef _UTILS_PROFILEDUMP_HPP_
#define _UTILS_PROFILEDUMP_HPP_

#include <map>
#include <set>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>
#include <Generics/Time.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>

using namespace AdServer::UserInfoSvcs;

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  Application_() noexcept;

  virtual ~Application_() noexcept;

  void main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  typedef std::list<std::string> FileNameList;

  enum ProfileType
  {
    PT_BASE,
    PT_ADD,
    PT_HISTORY,
    PT_ACTION,
    PT_CAMPAIGNREACH,
    PT_INVENTORY,
    PT_REQUEST
  };

protected:
  static void
  print_profile_(
    ProfileType type,
    const char* user_id,
    const FileNameList& files)
    /*throw(eh::Exception)*/;

  static void
  print_plain_(
    std::ostream& ostr,
    const void* buf,
    unsigned long size,
    const char* prefix)
    noexcept;

  static void
  print_profile_from_file_(
    ProfileType type,
    const char* user_id,
    const char* filename)
    /*throw(eh::Exception)*/;

  static void
  print_profile_from_block_(
    ProfileType type,
    const char* user_id,
    const Generics::MemBuf& buf)
    noexcept;

  static void
  dump_request_profiles_(const FileNameList& files) noexcept;
};

typedef Generics::Singleton<Application_> Application;

#endif /*_UTILS_PROFILEDUMP_HPP_*/
