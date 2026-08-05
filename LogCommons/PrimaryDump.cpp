#include <limits>
#include <sys/statvfs.h>
#include <utility>

#include "PrimaryDump.hpp"

namespace AdServer::LogProcessing
{
  PrimaryDump::PrimaryDump(
    std::string check_space_filesystem,
    std::uint64_t min_free_space,
    std::string suffix,
    const Generics::Time& disable_period)
    : check_space_filesystem_(std::move(check_space_filesystem)),
      min_free_space_(min_free_space),
      suffix_(std::move(suffix)),
      disable_period_(disable_period)
  {}

  bool
  PrimaryDump::available() const noexcept
  {
    {
      const std::lock_guard guard(lock_);
      if (Generics::Time::get_time_of_day() < disabled_until_)
      {
        return false;
      }
    }

    struct statvfs stat;
    if (::statvfs(check_space_filesystem_.c_str(), &stat) != 0)
    {
      disable();
      return false;
    }

    if (stat.f_files != 0 && stat.f_favail == 0)
    {
      return false;
    }

    const auto fragment_size = static_cast<std::uint64_t>(stat.f_frsize);
    const auto available_blocks = static_cast<std::uint64_t>(stat.f_bavail);
    const std::uint64_t available_space =
      fragment_size != 0 &&
        available_blocks > std::numeric_limits<std::uint64_t>::max() / fragment_size ?
      std::numeric_limits<std::uint64_t>::max() :
      available_blocks * fragment_size;

    return available_space != 0 && available_space >= min_free_space_;
  }

  void
  PrimaryDump::disable() const noexcept
  {
    const std::lock_guard guard(lock_);
    disabled_until_ = Generics::Time::get_time_of_day() + disable_period_;
  }

  std::string
  PrimaryDump::output_dir(const std::string& secondary_output_dir) const
  {
    return secondary_output_dir + suffix_;
  }
}
