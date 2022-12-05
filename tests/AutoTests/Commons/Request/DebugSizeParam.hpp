
#ifndef _AUTOTESTS_COMMONS_REQUEST_DEBUGSIZEPARAM_HPP
#define _AUTOTESTS_COMMONS_REQUEST_DEBUGSIZEPARAM_HPP

#include "BaseRequest.hpp"

namespace AutoTest
{
  /**
   * @brief debug size complex param
   *
   * Allows to set debug size for indicated/all  banner(s).
   */
  template<typename Request>
  class DebugSizeParam:
    public ComplexParam<Request, unsigned long>
  {
    typedef ComplexParam<Request, unsigned long> Base;

  public:
    template<class T>
    DebugSizeParam(
      Request* request,
      const char* name,
      const T& defs,
      bool set_defs = true);

    DebugSizeParam(Request* request, const char* name);

    DebugSizeParam(Request* request, const DebugSizeParam& other);

    template<class T>
    DebugSizeParam& operator= (const T& val);

    virtual bool
    print(
      std::ostream& out,
      const char* prefix,
      const char* eql) const;

    virtual ~DebugSizeParam() noexcept;
  };
};

#include "DebugSizeParam.tpp"

#endif  // _AUTOTESTS_COMMONS_REQUEST_DEBUGSIZEPARAM_HPP

