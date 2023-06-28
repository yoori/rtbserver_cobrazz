#ifndef USER_INFO_SVCS_USER_BIND_ADMIN_APPLICATION_HPP
#define USER_INFO_SVCS_USER_BIND_ADMIN_APPLICATION_HPP

// UNIX_COMMONS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>

class Application final
  : protected Generics::Uncopyable
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  Application() = default;

  ~Application() = default;

  void run(int argc, char** argv);
};

#endif /*USER_INFO_SVCS_USER_BIND_ADMIN_APPLICATION_HPP*/
