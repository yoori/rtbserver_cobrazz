#include "UrlInfoStorage.hpp"

#include <filesystem>
#include <stdexcept>

namespace AdServer::RequestInfoSvcs
{
  UrlInfoStorage::UrlInfoStorage(
    const std::string& path, unsigned long workers, unsigned long batch_size)
  {
    if (path.empty() || workers == 0 || batch_size == 0)
    {
      throw std::invalid_argument("Storage path, workers and batch_size must be nonempty/nonzero");
    }

    std::filesystem::create_directories(path);
    profiles_ = new ProfileMap(path, Generics::Time::ZERO, workers, batch_size);
    add_child_object(profiles_);
  }

  UrlInfoStorage::~UrlInfoStorage() noexcept = default;

  Commons::StartableAwaitable<std::string> UrlInfoStorage::get_url_profile(std::string url) const
  {
    if (url.empty())
    {
      throw std::invalid_argument("url must not be empty");
    }

    const auto profile = co_await profiles_->co_get_profile(url);
    co_return profile ?
      std::string(profile->membuf().get<char>(), profile->membuf().size()) : std::string();
  }

  Commons::StartableAwaitable<void>
  UrlInfoStorage::set_url_profile(std::string url, std::string text) const
  {
    if (url.empty())
    {
      throw std::invalid_argument("url must not be empty");
    }

    Generics::ConstSmartMemBuf_var profile =
      new Generics::ConstSmartMemBuf(text.data(), text.size());
    co_await profiles_->co_save_profile(url, profile);
  }
}
