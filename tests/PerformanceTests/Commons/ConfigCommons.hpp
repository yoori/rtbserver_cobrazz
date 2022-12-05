
#ifndef __CONFIGCOMMONS_HPP
#define __CONFIGCOMMONS_HPP

#include "ListSelectorPolicy.hpp"

// Aliases for useful common types

/**
 * @class ConstraintConfig
 * @brief Performance test constraint. Describe test's completion conditions.
 */
class  ConstraintConfig :
  public virtual ReferenceCounting::Interface,
  public virtual ReferenceCounting::DefaultImpl<>
{
public:

  /**
   * @brief Constructor.
   *
   * @param constraint XML-constraint given by XML-parser (Xerces).
   */
  ConstraintConfig(const requestConstraintType& constraint) :
    timeout(constraint.timeout().get()),
    intended_time(constraint.intendedTime().get()),
    failed_percentage(constraint.failedPercentage().get()),
    prolonged_percentage(constraint.prolongedPercentage().get()),
    sampling_size(constraint.samplingSize().get())
  {
  }

  const time_t timeout;                     // timeout for waiting server response
  const unsigned long intended_time;        // maximal expected server response's delay
  const unsigned long failed_percentage;    // percentage of failed response (invalid status or timed-out)
  const unsigned long prolonged_percentage; // percentage of prolonged (duration > intended_time) response
  const unsigned long sampling_size;        // sample for check conditions
  
};

typedef ReferenceCounting::SmartPtr<ConstraintConfig> ConstraintConfig_var;


/**
 * @class RequestConfig
 * @brief Base class for performance test requests config
 */
class RequestConfig
{
    
public:

  /**
   * @brief Constructor.
   *
   * @param _constraint constraint for request.
   * @param _url Base URL for request.
   * @param _method HTTP-method for request.
   */  
  RequestConfig(ConstraintConfig* _constraint,
                const char* _url = "",
                const char* _method = "");

  /**
   * @brief Access to request parameters.
   *
   * @return reference to parameters selector
   */
  const SelectorPolicyList& parameters() const;

  /**
   * @brief Access to request headers.
   *
   * @return reference to headers selector
   */
  const SelectorPolicyList& headers()  const;

  /**
   * @brief Access to request cookies.
   *
   * @return reference to cookies selector
   */
  const SelectorPolicyList& cookies()  const;

  const std::string url;                  // request base URL
  const std::string method;               // HTTP-method using for request
  const ConstraintConfig_var constraint;  // constraint for request.
  
protected:
  SelectorPolicyList parameters_;         // parameters selector
  SelectorPolicyList headers_;            // headers selector
  SelectorPolicyList cookies_;            // cookies selector
};

typedef ClientRequestType::requestConstraint_type client_constraint;
typedef OptOutType::requestConstraint_type optout_constraint;
typedef GeneratedRequestType::requestConstraint_type generated_request_constraint;

/**
 * @class ParamsRequestConfig
 * @brief Request with params config
 */
class ParamsRequestConfig : public RequestConfig
{

  static const char* COOKIE_HEADER; // Cookie's header name

public:
  /**
   * @brief Constructor.
   *
   * @param _constraint constraint for request.
   * @param request XML-presentation of request config.
   * @param request_lists access to parameters/headers/cookies values.
   */  
  ParamsRequestConfig(ConstraintConfig* _constraint,
                      const RequestType& request,
                      const RequestLists& request_lists)
      /*throw(SelectorPolicy::InvalidConfigRequestData)*/;
};

typedef std::unique_ptr<RequestConfig> RequestConfig_var;


#endif  // __CONFIGCOMMONS_HPP
