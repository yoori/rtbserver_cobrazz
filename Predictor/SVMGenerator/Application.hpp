#ifndef SVMGENERATOR_APPLICATION_HPP_
#define SVMGENERATOR_APPLICATION_HPP_

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
  generate_svm_(
    std::ostream& out,
    std::istream& in,
    const char* config_file,
    const char* feature_columns_str,
    const char* dictionary_file_path,
    bool check_features);

  void
  load_dictionary_(
    std::map<std::string, std::string>& dict,
    const char* file);
};

typedef Generics::Singleton<Application_> Application;

#endif /*SVMGENERATOR_APPLICATION_HPP_*/
