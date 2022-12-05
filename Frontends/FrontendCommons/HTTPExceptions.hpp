
#pragma once
#include <eh/Exception.hpp>

namespace FrontendCommons
{
  /**
   * Front-ends have constraints for serving HTTP requests and parameters
   * in HTTP requests. Some structs reused while checking this constraints.
   * HTTPExceptions control exceptions declaration.
   */

  struct HTTPExceptions
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidParamException, Exception);
    DECLARE_EXCEPTION(ForbiddenException, Exception);
    // Use if need separate case when size of parameter is correct,
    // but value is invalid
    DECLARE_EXCEPTION(BadParameter, Exception);
  };
}
