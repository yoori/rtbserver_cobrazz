#ifndef UTILS_CTRGENERATOR_APPLICATION_HPP_
#define UTILS_CTRGENERATOR_APPLICATION_HPP_

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

#include <DTree/SVM.hpp>
#include <DTree/Label.hpp>

using namespace Vanga;

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  Application_() noexcept;

  virtual
  ~Application_() noexcept;

  void
  main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  void
  generate_svm_(
    std::ostream& out,
    std::istream& in,
    const char* column_names,
    const char* result_dictionary);
};

typedef Generics::Singleton<Application_> Application;

#endif /*UTILS_CTRGENERATOR_APPLICATION_HPP_*/
