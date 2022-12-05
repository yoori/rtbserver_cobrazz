
#include "ConfigCommons.hpp"

// Request config class
RequestConfig::RequestConfig(ConstraintConfig* _constraint,
                             const char* _url,
                             const char* _method) :
  url(_url),
  method(_method),
  constraint(_constraint)
{
}

const SelectorPolicyList& RequestConfig::parameters() const
{
  return parameters_;
}

const SelectorPolicyList& RequestConfig::headers() const
{
  return headers_;
}

const SelectorPolicyList& RequestConfig::cookies() const
{
  return cookies_;
}

// NSRequestConfig class

const char* ParamsRequestConfig::COOKIE_HEADER = "Cookie";

ParamsRequestConfig::ParamsRequestConfig(ConstraintConfig* _constraint,
                                         const RequestType& request,
                                         const RequestLists& request_lists) /*throw(SelectorPolicy::InvalidConfigRequestData)*/:
  RequestConfig(_constraint, request.url().get().c_str(), request.method().get().c_str())
{
  // Parameters
  unsigned long param_size = request.parameter().size();
  for (unsigned int i=0; i<param_size; i++)
    {
      std::string param_name  = request.parameter()[i].name();
      SelectorPolicy_var selector =
        SelectorPolicy_var(SelectorPolicy::make_policy(param_name,
                                                       request.parameter()[i],
                                                       request_lists,
                                                       request.parameter()[i].empty_prc()));
      parameters_.push_back(selector);
    }
  // Headers
  unsigned long header_size = request.header().size();
  for (unsigned int i=0; i<header_size; i++)
    {
      std::string header_name  = request.header()[i].name();
      if (header_name != COOKIE_HEADER)
        {
          SelectorPolicy_var selector =
            SelectorPolicy_var(SelectorPolicy::make_policy(header_name,
                                                           request.header()[i],
                                                           request_lists,
                                                           request.header()[i].empty_prc()));
          headers_.push_back(selector);
        }
      else
        {
          make_cookie_policy_list(cookies_,
                                  request.header()[i],
                                  request_lists);
        }
    }
}
