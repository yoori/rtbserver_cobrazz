#pragma once

#include <cstdint>

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer::RequestInfoSvcs
{
  constexpr std::uint32_t CURRENT_USER_NAVIGATION_PROFILE_VERSION = 2;

  struct UserNavigationProfileAdapter
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Generics::ConstSmartMemBuf_var
    operator()(const Generics::ConstSmartMemBuf* mem_buf) const;
  };
}
