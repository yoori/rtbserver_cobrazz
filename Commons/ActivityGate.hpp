#pragma once

#include <atomic>
#include <cstddef>

#include <Generics/ActiveObject.hpp>

namespace AdServer::Commons
{
  class ActivityGate final:
    public Generics::SimpleActiveObject
  {
  public:
    class Guard final
    {
    public:
      Guard() noexcept = default;

      explicit Guard(ActivityGate* gate) noexcept;

      ~Guard() noexcept;

      void reset() noexcept;

      Guard(const Guard&) = delete;
      Guard& operator=(const Guard&) = delete;

      Guard(Guard&& rhs) noexcept;
      Guard& operator=(Guard&& rhs) noexcept = delete;

      explicit operator bool() const noexcept;

    private:
      ActivityGate* gate_ = nullptr;
    };

  public:
    ActivityGate() = default;
    ~ActivityGate() noexcept override = default;

    Guard enter() noexcept;
    void wait_for_activities();
    bool has_activities() const noexcept;

  protected:
    void activate_object_() override;
    void deactivate_object_() override;
    bool wait_more_() override;

  private:
    void leave_() noexcept;

  private:
    std::atomic<bool> closed_{true};
    std::atomic<std::size_t> running_{0};
    std::atomic<std::size_t> waiters_{0};
  };
}
