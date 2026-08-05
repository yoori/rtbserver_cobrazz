#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include <Generics/Time.hpp>

namespace AdServer::LogProcessing
{
  class PrimaryDump final
  {
  public:
    PrimaryDump(
      std::string check_space_filesystem,
      std::uint64_t min_free_space,
      std::string suffix,
      const Generics::Time& disable_period = Generics::Time(60));

    bool
    available() const noexcept;

    void
    disable() const noexcept;

    std::string
    output_dir(const std::string& secondary_output_dir) const;

  private:
    const std::string check_space_filesystem_;
    const std::uint64_t min_free_space_;
    const std::string suffix_;
    const Generics::Time disable_period_;

    mutable std::mutex lock_;
    mutable Generics::Time disabled_until_;
  };

  using PrimaryDumpPtr = std::shared_ptr<PrimaryDump>;
}
