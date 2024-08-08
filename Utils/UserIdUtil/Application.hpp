#ifndef UTILS_USERIDUTIL_HPP
#define UTILS_USERIDUTIL_HPP

#include <iostream>
#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>
#include <String/SubString.hpp>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  Application_() noexcept;

  virtual ~Application_() noexcept;

  void main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  void
  uid_to_sspuid_(
    std::istream& in,
    std::ostream& out,
    const String::SubString& source_id,
    const String::SubString& global_key,
    const String::SubString& sep,
    unsigned long column)
    /*throw(InvalidArgument, eh::Exception)*/;

  void
  sspuid_to_uid_(
    std::istream& in,
    std::ostream& out,
    const String::SubString& source_id,
    const String::SubString& global_key,
    const String::SubString& sep,
    unsigned long column)
    /*throw(InvalidArgument, eh::Exception)*/;

  std::string
  uid_to_sspuid_(
    const AdServer::Commons::UserId& user_id)
    noexcept;

  void
  sign_uid_(
    std::istream& in,
    std::ostream& out,
    const String::SubString& private_key_file,
    const String::SubString& sep,
    unsigned long column)
    /*throw(InvalidArgument, eh::Exception)*/;

  void
  hex_to_uid_(
    std::istream& in,
    std::ostream& out);
};

typedef Generics::Singleton<Application_> Application;

#endif /*UTILS_USERIDUTIL_HPP*/
