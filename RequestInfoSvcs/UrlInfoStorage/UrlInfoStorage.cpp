#include "UrlInfoStorage.hpp"

#include <filesystem>
#include <stdexcept>
#include <Commons/Coro/CallbackAwaiter.hpp>

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
    const auto descriptions_path = path + "/descriptions";
    std::filesystem::create_directories(descriptions_path);
    descriptions_ = new ProfileMap(descriptions_path, Generics::Time::ZERO, workers, batch_size);
    add_child_object(descriptions_);
  }

  UrlInfoStorage::~UrlInfoStorage() noexcept = default;

  Commons::StartableAwaitable<std::string> UrlInfoStorage::get_url_profile(std::string url) const
  {
    return get_(*profiles_, std::move(url));
  }

  Commons::StartableAwaitable<void>
  UrlInfoStorage::set_url_profile(std::string url, std::string text) const
  {
    return set_(*profiles_, std::move(url), std::move(text));
  }

  Commons::StartableAwaitable<bool> UrlInfoStorage::compare_exchange_url_profile(
    std::string url, std::string expected, std::string text) const
  {
    return compare_exchange_(*profiles_, std::move(url), std::move(expected), std::move(text));
  }

  Commons::StartableAwaitable<std::string>
  UrlInfoStorage::get_url_description(std::string url) const
  {
    return get_(*descriptions_, std::move(url));
  }

  Commons::StartableAwaitable<void>
  UrlInfoStorage::set_url_description(std::string url, std::string text) const
  {
    return set_(*descriptions_, std::move(url), std::move(text));
  }

  Commons::StartableAwaitable<bool> UrlInfoStorage::compare_exchange_url_description(
    std::string url, std::string expected, std::string text) const
  {
    return compare_exchange_(*descriptions_, std::move(url), std::move(expected), std::move(text));
  }

  Commons::StartableAwaitable<std::string>
  UrlInfoStorage::get_(ProfileMap& map, std::string url) const
  {
    if (url.empty())
    {
      throw std::invalid_argument("url must not be empty");
    }

    const auto profile = co_await map.co_get_profile(url);
    co_return profile ?
      std::string(profile->membuf().get<char>(), profile->membuf().size()) : std::string();
  }

  Commons::StartableAwaitable<void>
  UrlInfoStorage::set_(ProfileMap& map, std::string url, std::string text) const
  {
    if (url.empty())
    {
      throw std::invalid_argument("url must not be empty");
    }

    Generics::ConstSmartMemBuf_var profile =
      new Generics::ConstSmartMemBuf(text.data(), text.size());
    co_await Commons::CallbackAwaiter<>(
      [this, &url](auto ready) { acquire_write_(url, std::move(ready)); });
    WriteGuard guard{*this, url};
    co_await map.co_save_profile(url, profile);
  }

  Commons::StartableAwaitable<bool> UrlInfoStorage::compare_exchange_(
    ProfileMap& map, std::string url, std::string expected, std::string text) const
  {
    if (url.empty())
    {
      throw std::invalid_argument("url must not be empty");
    }

    co_await Commons::CallbackAwaiter<>(
      [this, &url](auto ready) { acquire_write_(url, std::move(ready)); });
    WriteGuard guard{*this, url};
    if (co_await get_(map, url) != expected)
    {
      co_return false;
    }

    Generics::ConstSmartMemBuf_var profile =
      new Generics::ConstSmartMemBuf(text.data(), text.size());
    co_await map.co_save_profile(url, profile);
    co_return true;
  }

  void UrlInfoStorage::acquire_write_(const std::string& url, std::function<void()> ready) const
  {
    {
      std::lock_guard lock(writes_mutex_);
      auto [position, inserted] = writes_.try_emplace(url);
      if (!inserted)
      {
        position->second.push_back(std::move(ready));
        return;
      }
    }
    ready();
  }

  void UrlInfoStorage::release_write_(const std::string& url) const
  {
    std::function<void()> ready;
    {
      std::lock_guard lock(writes_mutex_);
      auto position = writes_.find(url);
      if (position->second.empty())
      {
        writes_.erase(position);
      }
      else
      {
        ready = std::move(position->second.front());
        position->second.pop_front();
      }
    }
    if (ready)
    {
      ready();
    }
  }

  UrlInfoStorage::WriteGuard::~WriteGuard()
  {
    owner.release_write_(url);
  }
}
