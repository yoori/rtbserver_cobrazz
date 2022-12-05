#ifndef PREDICTOR_GEOUTIL_APPLICATION_HPP_
#define PREDICTOR_GEOUTIL_APPLICATION_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>
#include <Generics/Time.hpp>
#include <Commons/Containers.hpp>

class Application_
{
public:
  Application_() noexcept;

  virtual
  ~Application_() noexcept;

  void
  main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

protected:
  void
  add_geo_(
    std::istream& in,
    std::ostream& out);
};

typedef Generics::Singleton<Application_> Application;

#endif /*PREDICTOR_GEOUTIL_APPLICATION_HPP_*/
