#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <Commons/Coro/SetAwaitable.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>

namespace
{
  constexpr std::string_view ERROR_MESSAGE = "test exception";

  AdServer::Commons::StartableAwaitable<int>
  make_result(bool fail)
  {
    if (fail)
    {
      throw std::runtime_error(std::string(ERROR_MESSAGE));
    }

    co_return 42;
  }

  AdServer::Commons::StartableAwaitable<void>
  make_void_result(bool fail)
  {
    if (fail)
    {
      throw std::runtime_error(std::string(ERROR_MESSAGE));
    }

    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  make_batch(bool fail)
  {
    std::vector<AdServer::Commons::StartableAwaitable<void>> operations;
    operations.emplace_back(make_void_result(false));
    operations.emplace_back(make_void_result(fail));
    co_await AdServer::Commons::SetAwaitable(std::move(operations));
  }

  template<typename Function>
  bool
  check_exception(Function&& function, std::string_view test_name)
  {
    try
    {
      function();
    }
    catch (const std::runtime_error& ex)
    {
      if (ex.what() == ERROR_MESSAGE)
      {
        return true;
      }

      std::cerr << test_name << ": unexpected error: " << ex.what() << '\n';
      return false;
    }
    catch (...)
    {
      std::cerr << test_name << ": unexpected exception type\n";
      return false;
    }

    std::cerr << test_name << ": exception was not propagated\n";
    return false;
  }
}

int
main()
{
  if (AdServer::Commons::sync_wait(make_result(false)) != 42)
  {
    std::cerr << "result sync_wait returned an unexpected value\n";
    return 1;
  }

  AdServer::Commons::sync_wait(make_void_result(false));

  if (!check_exception(
      []()
      {
        AdServer::Commons::sync_wait(make_result(true));
      },
      "result sync_wait"))
  {
    return 1;
  }

  if (!check_exception(
      []()
      {
        AdServer::Commons::sync_wait(make_void_result(true));
      },
      "void sync_wait"))
  {
    return 1;
  }

  AdServer::Commons::sync_wait(make_batch(false));
  if (!check_exception(
      []()
      {
        AdServer::Commons::sync_wait(make_batch(true));
      },
      "batch sync_wait"))
  {
    return 1;
  }

  std::cout << "StartableAwaitableTest: PASS\n";
  return 0;
}
