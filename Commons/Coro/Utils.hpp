#pragma once

#include <coroutine>
#include <deque>

namespace AdServer::Commons
{
  inline void
  resume_coroutine(std::coroutine_handle<> handle)
  {
    thread_local bool draining = false;
    thread_local std::deque<std::coroutine_handle<>> pending;

    pending.push_back(handle);
    if(draining)
    {
      return;
    }

    draining = true;
    while(!pending.empty())
    {
      std::coroutine_handle<> next = pending.front();
      pending.pop_front();
      next.resume();
    }
    draining = false;
  }
}
