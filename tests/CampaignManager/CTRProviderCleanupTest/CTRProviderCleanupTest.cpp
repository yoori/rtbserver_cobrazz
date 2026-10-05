#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <system_error>

#include <CampaignSvcs/CampaignManager/CTRProvider.hpp>

namespace
{
  class TempDirectory
  {
  public:
    TempDirectory()
    {
      std::array<char, 64> path{};
      const std::string pattern =
        (std::filesystem::temp_directory_path() / "CTRProviderCleanupTest.XXXXXX").string();
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
}

int main()
{
  try
  {
    TempDirectory temp_directory;
    const std::filesystem::path model_directory = temp_directory.path() / "model.VTR";
    const std::filesystem::path nested_directory = model_directory / "traits" / "campaigns";
    std::filesystem::create_directories(nested_directory);
    std::ofstream(nested_directory / "1.json") << "{}\n";

    const std::filesystem::path standalone_file = temp_directory.path() / "standalone.json";
    std::ofstream(standalone_file) << "{}\n";

    AdServer::CampaignSvcs::CTR::CTRProvider::remove_config_paths(
      std::list<std::string>{standalone_file.string()},
      std::list<std::string>{model_directory.string()});

    if (std::filesystem::exists(model_directory) || std::filesystem::exists(standalone_file))
    {
      throw std::runtime_error("Config paths were not removed recursively");
    }
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
