
#include <tests/AutoTests/Commons/Common.hpp>

#ifndef _USERPROFILING_COMMONS_HPP
#define _USERPROFILING_COMMONS_HPP


namespace AutoTest
{
  namespace UserProfiling
  {

    // Utils

    /**
     * @brief prepare advertising request.
     * @param test unit.
     * @param advertising request.
     * @param request parameters.
     * @param using debug time sign.
     */
    template<typename RequestType>
    void make_request(
      BaseUnit* test,
      NSLookupRequest& request,
      const RequestType& params,
      const AutoTest::Time& base_time = Generics::Time::ZERO)
    {
      if (params.referer_kw)
      {
        std::string refererkw_names(params.referer_kw);
        std::string referer_kw;
        String::StringManip::SplitComma tokenizer(refererkw_names);
        String::SubString token;
        while (tokenizer.get_token(token))
        {
          String::StringManip::trim(token);
          if (!referer_kw.empty()) referer_kw+=",";
          referer_kw += test->fetch_string(token.str());
        }
        
        request.referer_kw = referer_kw;
      }
      if (base_time != Generics::Time::ZERO)
      {
        request.debug_time = base_time + params.time_ofset;
      }
    }
    
  }
}


#endif  // _USERPROFILING_COMMONS_HPP
