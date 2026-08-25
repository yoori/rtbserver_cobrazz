#pragma once

#include <map>
#include <set>
#include <string>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <RequestInfoManagerGrpc.grpc-client.hpp>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidParam, Exception);

  Application_() noexcept;

  virtual ~Application_() noexcept;

  int main(int& argc, char** argv) noexcept;

protected:
  using Client = AdServer::RequestInfoSvcs::
    RequestInfoManagerGrpcAsyncClient;

  void check_option_(
    const char *opt_name,
    Generics::AppUtils::Option<std::string>& option,
    const std::string& opt_value = std::string())
    /*throw(InvalidParam)*/;

  int print(
    Client& request_info_manager,
    const char* request_id_str,
    bool print_plain)
    noexcept;

  int print_user_campaign_reach(
    Client& request_info_manager,
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
    Client& request_info_manager,
    const char* user_id_str,
    bool print_plain,
    bool align)
    noexcept;

  int
  print_user_fraud_protection(
    Client& request_info_manager,
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
    Client& request_info_manager,
    const char* request_id_str,
    bool print_plain)
    noexcept;

  int
  print_user_site_reach(
    Client& request_info_manager,
    const char* user_id_str,
    bool print_plain)
    noexcept;

  int print_user_tag_request_group(
    Client& request_info_manager,
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
