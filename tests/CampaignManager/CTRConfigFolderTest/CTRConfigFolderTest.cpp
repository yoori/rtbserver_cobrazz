#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

#include <CampaignSvcs/CampaignManager/CTRConfigSelector.hpp>

namespace
{
  class TempDirectory
  {
  public:
    TempDirectory()
    {
      std::array<char, 64> path{};
      const std::string pattern =
        (std::filesystem::temp_directory_path() / "CTRConfigFolderTest.XXXXXX").string();
      if (pattern.size() >= path.size())
      {
        throw std::runtime_error("Temporary directory path is too long");
      }
      std::copy(pattern.begin(), pattern.end(), path.begin());
      const char* result = ::mkdtemp(path.data());
      if (!result)
      {
        throw std::runtime_error("Can't create temporary directory");
      }
      path_ = result;
    }

    ~TempDirectory()
    {
      std::error_code error;
      std::filesystem::remove_all(path_, error);
    }

    const std::filesystem::path& path() const noexcept
    {
      return path_;
    }

  private:
    std::filesystem::path path_;
  };

  void create_directory(const std::filesystem::path& root, const char* name)
  {
    if (!std::filesystem::create_directory(root / name))
    {
      throw std::runtime_error("Can't create test directory");
    }
  }

  void check_selected_folder(
    const std::filesystem::path& root,
    const char* expected_name,
    const char* expected_timestamp)
  {
    std::string selected_root;
    const Generics::Time timestamp =
      AdServer::CampaignSvcs::CTR::select_latest_config(selected_root, root.string());
    if (selected_root != (root / expected_name).string())
    {
      throw std::runtime_error("Unexpected selected model directory: " + selected_root);
    }
    const Generics::Time expected(std::string(expected_timestamp), "%Y%m%d.%H%M%S");
    if (timestamp != expected)
    {
      throw std::runtime_error("Unexpected selected model timestamp");
    }
  }
}

int main()
{
  try
  {
    TempDirectory directory;
    create_directory(directory.path(), "20260913.120000");
    create_directory(directory.path(), "20260913.130000.CTR");
    create_directory(directory.path(), "20260913.140000.VTR");
    create_directory(directory.path(), "20260913.150000.SSP-CTR-CHECK");
    create_directory(directory.path(), "invalid");
    check_selected_folder(
      directory.path(), "20260913.140000.VTR", "20260913.140000");

    const std::filesystem::path ctr_root = directory.path() / "ctr";
    create_directory(directory.path(), "ctr");
    create_directory(ctr_root, "20260913.150000.CTR");
    check_selected_folder(ctr_root, "20260913.150000.CTR", "20260913.150000");

    const std::filesystem::path legacy_root = directory.path() / "legacy";
    create_directory(directory.path(), "legacy");
    create_directory(legacy_root, "20260913.160000");
    check_selected_folder(legacy_root, "20260913.160000", "20260913.160000");
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
