#ifndef __QUERYSENDERBASE_HPP
#define __QUERYSENDERBASE_HPP

#include "StatCommons.hpp"
#include <HTTP/HttpAsync.hpp>

class QuerySenderBase
{
public:
  /**
   * @brief Callback calling from BaseRequest after getting correct response.
   */
  virtual void
  on_response(
    unsigned long client_id,
    const HTTP::ResponseInformation& data,
    bool is_opted_out,
    unsigned long ccid = 0,
    const AdvertiserResponse*
    ad_response = 0) noexcept = 0;

  /**
   * @brief Callback calling from BaseRequest after getting HTTP error.
   */
  virtual void on_error(
    const String::SubString& description,
    const HTTP::ResponseInformation& data,
    bool is_opted_out) noexcept = 0;

  /**
   * Destructor
   */
  virtual ~QuerySenderBase() noexcept = default;
};

#endif  // __QUERYSENDERBASE_HPP
