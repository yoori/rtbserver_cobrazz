#pragma once

#include <cstdint>

#include <Generics/Time.hpp>
#include <ProfilingCommons/FileController.hpp>

namespace AdServer
{
namespace ProfilingCommons
{
  struct LevelMapTraits
  {
    enum Mode
    {
      BLOCK_RUNTIME = 0,
      NONBLOCK_RUNTIME,
    };

    LevelMapTraits(
      Mode mode_val,
      unsigned long rw_buffer_size_val,
      uint64_t rwlevel_max_size_val,
      uint64_t max_undumped_size_val,
      unsigned long max_levels0_val,
      const Generics::Time& expire_time_val,
      FileController* file_controller_val = nullptr)
      noexcept
      : mode(mode_val),
        rw_buffer_size(rw_buffer_size_val),
        rwlevel_max_size(rwlevel_max_size_val),
        max_undumped_size(max_undumped_size_val),
        max_levels0(max_levels0_val),
        expire_time(expire_time_val),
        file_controller(ReferenceCounting::add_ref(file_controller_val))
    {}

    Mode mode;
    unsigned long rw_buffer_size;
    uint64_t rwlevel_max_size;
    uint64_t max_undumped_size;
    unsigned long max_levels0;
    Generics::Time expire_time;
    FileController_var file_controller;
  };
}
}
