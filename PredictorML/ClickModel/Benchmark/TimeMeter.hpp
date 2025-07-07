#ifndef CLICKMODEL_TIMEMETER_HPP
#define CLICKMODEL_TIMEMETER_HPP

// STD
#include <chrono>
#include <cstdint>

class TimeMeter final
{
public:
  using TimePoint = std::chrono::time_point<
    std::chrono::high_resolution_clock>;

public:
  TimeMeter() = default;

  void start()
  {
    start_time_ = std::chrono::high_resolution_clock::now();
  }

  std::uint64_t stop()
  {
    auto end_time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time_).count();
  }

private:
  TimePoint start_time_;
};

#endif //CLICKMODEL_TIMEMETER_HPP
