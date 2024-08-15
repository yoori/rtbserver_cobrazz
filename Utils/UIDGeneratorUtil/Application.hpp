#ifndef UTILS_UIDGENERATORUTIL_APPLICATION_HPP_
#define UTILS_UIDGENERATORUTIL_APPLICATION_HPP_

#include <arpa/inet.h>
#include <map>
#include <set>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>
#include <Generics/Time.hpp>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  Application_() noexcept;

  virtual ~Application_() noexcept;

  void main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  void
  generate_request_(
    std::ostream& out,
    const String::SubString& ids_str,
    const String::SubString& buckets_str,
    const String::SubString& obuckets_str)
    noexcept;

  void
  send_(
    const String::SubString& socket_str,
    const Generics::Time& timeout,
    unsigned long max_portion)
    /*throw (eh::Exception)*/;

  void
  generate_request_buf_(
    std::vector<unsigned char>& buf,
    const String::SubString& ids_str,
    const String::SubString& buckets_str,
    const String::SubString& obuckets_str)
    noexcept;
};

typedef Generics::Singleton<Application_> Application;

#endif /*UTILS_UIDGENERATORUTIL_APPLICATION_HPP_*/
