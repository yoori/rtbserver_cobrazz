#include <filesystem>
#include <string>

// GCC 10 reports the provisional C++20 value (201709L).
static_assert(__cplusplus > 201703L);
static_assert(_GLIBCXX_USE_CXX11_ABI == 1);

extern "C" bool check_filesystem()
{
  const std::filesystem::path path("/sys/fs/cgroup/cpu.max");
  return path.parent_path().string() == "/sys/fs/cgroup";
}
