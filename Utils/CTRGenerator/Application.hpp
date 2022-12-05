#ifndef UTILS_CTRGENERATOR_APPLICATION_HPP_
#define UTILS_CTRGENERATOR_APPLICATION_HPP_

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
  generate_model_(
    std::ostream& out,
    const char* out_weight_file,
    const char* config_file,
    const char* config_data_file);

  void
  generate_svm_(
    std::ostream& out,
    std::istream& in,
    const char* config_file,
    const char* feature_columns_str,
    const char* cc_to_ccg_dictionary_file_path,
    const char* cc_to_campaign_dictionary_file_path,
    const char* tag_to_publisher_dictionary_file_path,
    const char* dictionary_file,
    const char* name_dictionary_file);

  void
  generate_ctr_(
    std::ostream& out,
    std::istream& in,
    const char* config_dir,
    const char* feature_columns_str,
    const char* cc_to_ccg_dictionary_file_path,
    const char* cc_to_campaign_dictionary_file_path,
    const char* tag_to_publisher_dictionary_file_path,
    bool out_hashes);

  void
  generate_xgb_ctr_(
    std::ostream& out,
    std::istream& in,
    const char* xgb_model_file,
    const char* config_file,
    const char* feature_columns_str,
    const char* cc_to_ccg_dictionary_file_path,
    const char* cc_to_campaign_dictionary_file_path,
    const char* tag_to_publisher_dictionary_file_path,
    bool out_hashes);

  void
  load_dictionary_(
    std::map<std::string, std::string>& dict,
    const char* file);
};

typedef Generics::Singleton<Application_> Application;

#endif /*UTILS_CTRGENERATOR_APPLICATION_HPP_*/
