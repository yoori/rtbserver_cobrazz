#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include <boost/intrusive/set.hpp>

#include <Generics/Time.hpp>

namespace AdServer::Commons
{
  class FastScheduler final
  {
  public:
    using Callback = std::optional<Generics::Time> (*)(void*) noexcept;

    struct Task
    {
      boost::intrusive::set_member_hook<> timer_hook;
      Task* pending_next = nullptr;
      Generics::Time deadline;
      Callback callback = nullptr;
      std::shared_ptr<void> owner;
    };

    explicit FastScheduler(std::size_t threads);

    ~FastScheduler() noexcept;

    FastScheduler(const FastScheduler&) = delete;
    FastScheduler& operator=(const FastScheduler&) = delete;

    void schedule(
      Task& task,
      const Generics::Time& deadline,
      std::shared_ptr<void> owner,
      Callback callback) noexcept;

    template<typename Owner, typename CallbackType>
      requires (
        std::is_empty_v<CallbackType> &&
        std::is_nothrow_invocable_v<CallbackType, Owner&>)
    void schedule(
      Task& task,
      const Generics::Time& deadline,
      std::shared_ptr<Owner> owner,
      CallbackType) noexcept;

  private:
    class Impl;

    std::unique_ptr<Impl> impl_;
  };

  template<typename Owner, typename CallbackType>
    requires (
      std::is_empty_v<CallbackType> &&
      std::is_nothrow_invocable_v<CallbackType, Owner&>)
  void FastScheduler::schedule(
    Task& task,
    const Generics::Time& deadline,
    std::shared_ptr<Owner> owner,
    CallbackType) noexcept
  {
    using Result = std::invoke_result_t<CallbackType, Owner&>;
    static_assert(
      std::is_void_v<Result> ||
      std::is_same_v<Result, std::optional<Generics::Time>>);

    schedule(
      task,
      deadline,
      std::shared_ptr<void>(std::move(owner)),
      static_cast<Callback>(
        [](void* opaque_owner) noexcept -> std::optional<Generics::Time>
        {
          auto& typed_owner = *static_cast<Owner*>(opaque_owner);
          if constexpr (std::is_void_v<Result>)
          {
            CallbackType{}(typed_owner);
            return std::nullopt;
          }
          else
          {
            return CallbackType{}(typed_owner);
          }
        }));
  }
}
