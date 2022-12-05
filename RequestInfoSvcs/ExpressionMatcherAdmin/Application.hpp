#ifndef _LOG_PROCESSING_EXPRESSION_MATCHER_ADMIN_APPLICATION_HPP_
#define _LOG_PROCESSING_EXPRESSION_MATCHER_ADMIN_APPLICATION_HPP_

#include <map>
#include <set>
#include <string>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/ExpressionMatcher.hpp>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  Application_() noexcept;

  virtual ~Application_() noexcept;

  void main(int& argc, char** argv) noexcept;

protected:
  AdServer::RequestInfoSvcs::ExpressionMatcher*
  resolve_expression_matcher(
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    const CORBACommons::CorbaObjectRef& corba_ref)
    /*throw(eh::Exception, Exception, CORBA::SystemException)*/;

  void print(
    AdServer::RequestInfoSvcs::ExpressionMatcher* expression_matcher,
    const char* user_id_str)
    noexcept;

  void print_estimation(
    AdServer::RequestInfoSvcs::ExpressionMatcher* expression_matcher,
    const char* user_id_str)
    noexcept;

  void print_user_trigger_match(
    AdServer::RequestInfoSvcs::ExpressionMatcher* expression_matcher,
    const char* user_id_str,
    bool temporary)
    noexcept;

  void print_request_trigger_match(
    AdServer::RequestInfoSvcs::ExpressionMatcher* expression_matcher,
    const char* user_id_str)
    noexcept;

  void
  print_household_colo_reach(
    AdServer::RequestInfoSvcs::ExpressionMatcher* expression_matcher,
    const char* user_id_str)
    noexcept;
};

typedef Generics::Singleton<Application_> Application;

#endif /*_LOG_PROCESSING_EXPRESSION_MATCHER_ADMIN_APPLICATION_HPP_*/
