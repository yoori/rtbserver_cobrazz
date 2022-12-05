#ifndef _REQUEST_INFO_SVCS_REQUEST_INFO_ADMIN_APPLICATION_HPP_
#define _REQUEST_INFO_SVCS_REQUEST_INFO_ADMIN_APPLICATION_HPP_

#include <map>
#include <set>
#include <string>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <RequestInfoSvcs/RequestInfoManager/RequestInfoManager.hpp>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidParam, Exception);

  Application_() noexcept;

  virtual ~Application_() noexcept;

  int main(int& argc, char** argv) noexcept;

protected:
  void check_option_(
    const char *opt_name,
    Generics::AppUtils::Option<std::string>& option,
    const std::string& opt_value = std::string())
    /*throw(InvalidParam)*/;

  int print(
    AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
    const char* request_id_str,
    bool print_plain)
    noexcept;

  int print_user_campaign_reach(
    AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
    const char* user_id_str,
    bool print_plain)
    noexcept;

  void
  print_user_action_buf_(
    const void* buf,
    unsigned long buf_size,
    bool print_plain,
    bool align)
    /*throw(eh::Exception)*/;
  
  int
  print_user_action(
    AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
    const char* user_id_str,
    bool print_plain,
    bool align)
    noexcept;

  int
  print_user_fraud_protection(
    AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
    const char* user_id_str,
    bool print_plain)
    noexcept;

  int
  print_user_action_from_file(
    const char* file,
    const char* user_id_str,
    bool print_plain,
    bool debug_plain,
    bool align)
    noexcept;

  int print_passback(
    AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
    const char* request_id_str,
    bool print_plain)
    noexcept;

  int
  print_user_site_reach(
    AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
    const char* user_id_str,
    bool print_plain)
    noexcept;

  int print_user_tag_request_group(
    AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
    const char* user_id_str,
    bool print_plain)
    noexcept;

  void
  print_plain_(
    std::ostream& ostr,
    const void* buf,
    unsigned long size,
    const char* prefix = "")
    noexcept;
};

typedef Generics::Singleton<Application_> Application;

#endif /*_REQUEST_INFO_SVCS_REQUEST_INFO_ADMIN_APPLICATION_HPP_*/
