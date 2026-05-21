#pragma once

#include <algorithm>
#include <cstddef>
#include <string_view>

#if defined(__linux__)
#include <pthread.h>
#endif

namespace AdServer::Commons
{
  inline void
  set_current_thread_name(std::string_view name) noexcept
  {
#if defined(__linux__)
    if (!name.empty())
    {
      char thread_name[16] = {};
      const auto size = std::min<std::size_t>(
        name.size(),
        sizeof(thread_name) - 1);
      name.copy(thread_name, size);
      pthread_setname_np(pthread_self(), thread_name);
    }
#else
    (void)name;
#endif
  }
}
