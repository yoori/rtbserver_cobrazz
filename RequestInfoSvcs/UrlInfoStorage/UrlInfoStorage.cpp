#include "UrlInfoStorage.hpp"

#include <filesystem>
#include <stdexcept>
#include <openssl/sha.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <Logger/ActiveObjectCallback.hpp>
#include <Commons/Coro/CallbackAwaiter.hpp>

namespace AdServer::RequestInfoSvcs
{
  UrlInfoStorage::UrlInfoStorage(
    Logging::Logger* logger, const std::string& path, unsigned long workers,
    unsigned long batch_size)
    : Generics::RefCountableCompositeActiveObject(true),
      logger_(ReferenceCounting::add_ref(logger))
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
    Generics::ActiveObjectCallback_var callback =
      new Logging::ActiveObjectCallbackImpl(logger_, "UrlInfoStorage", "UrlInfoStorage");
    count_executor_ = std::make_shared<Commons::ExecutorPool>(
      callback, 1, Commons::ExecutorPool::ResumeStrategy::CurrentContext, "url-count");
    add_child_object(count_executor_);
    // Drain queued and running counts before stopping their executor and the profile maps.
    count_gate_ = std::make_shared<Commons::ActivityGate>();
    add_child_object(count_gate_);
  }

  UrlInfoStorage::~UrlInfoStorage() noexcept = default;

  Commons::StartableAwaitable<std::uint64_t> UrlInfoStorage::get_url_profile_count() const
  {
    return count_(*profiles_);
  }

  Commons::StartableAwaitable<std::uint64_t> UrlInfoStorage::get_url_description_count() const
  {
    return count_(*descriptions_);
  }

  Commons::StartableAwaitable<std::uint64_t> UrlInfoStorage::count_(ProfileMap& map) const
  {
    auto activity = count_gate_->enter();
    if (!activity)
    {
      throw std::runtime_error("UrlInfoStorage is stopping");
    }

    co_await Commons::ExecutorPool::yield(count_executor_);
    std::uint64_t count = 0;
    // size() is only a RocksDB estimate; an iterator counts distinct keys in its snapshot.
    map.process_keys_from(std::nullopt, [&count](const std::string&)
    {
      ++count;
      return true;
    });
    co_return count;
  }

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

  std::string UrlInfoStorage::description_version(std::string_view text)
  {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(text.data()), text.size(), digest);
    constexpr char hex[] = "0123456789abcdef";
    std::string result(SHA256_DIGEST_LENGTH * 2, '0');
    for (std::size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
      result[i * 2] = hex[digest[i] >> 4];
      result[i * 2 + 1] = hex[digest[i] & 15];
    }
    return result;
  }

  Commons::StartableAwaitable<std::optional<std::string>>
  UrlInfoStorage::order_url_description_full_indexing(
    std::string url, std::string version, std::string job_id) const
  {
    if (url.empty() || version.empty() || job_id.empty())
    {
      throw std::invalid_argument("url, version and job_id must not be empty");
    }

    co_await Commons::CallbackAwaiter<>(
      [this, &url](auto ready) { acquire_write_(url, std::move(ready)); });
    WriteGuard guard{*this, url};
    const auto text = co_await get_(*descriptions_, url);
    if (description_version(text) != version)
    {
      co_return std::nullopt;
    }

    google::protobuf::Struct value;
    if (!google::protobuf::util::JsonStringToMessage(text, &value).ok())
    {
      throw std::invalid_argument("description must be a JSON object");
    }

    auto& fields = *value.mutable_fields();
    const auto string_value = [&fields](const char* name) -> std::string
    {
      const auto it = fields.find(name);
      return it == fields.end() ? std::string() : it->second.string_value();
    };
    const auto success = fields.find("success");
    if (success == fields.end() || !success->second.bool_value())
    {
      throw std::invalid_argument("description is not ready");
    }

    if (string_value("fetch_mode") == "FULL" || string_value("scope") == "app")
    {
      co_return version;
    }

    if (string_value("fetch_mode") != "NON_FULL")
    {
      throw std::invalid_argument("invalid description fetch_mode");
    }

    auto& indexing = *fields["indexing"].mutable_struct_value()->mutable_fields();
    const auto state = indexing.find("state");
    if (state != indexing.end() && state->second.string_value() == "PASSED_TO_FULL_INDEXING")
    {
      co_return version;
    }
    indexing["state"].set_string_value("PASSED_TO_FULL_INDEXING");
    indexing["job_id"].set_string_value(job_id);
    std::string updated;
    const auto status = google::protobuf::util::MessageToJsonString(value, &updated);
    if (!status.ok())
    {
      throw std::runtime_error(status.ToString());
    }

    Generics::ConstSmartMemBuf_var profile =
      new Generics::ConstSmartMemBuf(updated.data(), updated.size());
    co_await descriptions_->co_save_profile(url, profile);
    co_return description_version(updated);
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
