#pragma once

#include <coroutine>

namespace AdServer::Commons
{
  void
  resume_coroutine(std::coroutine_handle<> handle);
}
