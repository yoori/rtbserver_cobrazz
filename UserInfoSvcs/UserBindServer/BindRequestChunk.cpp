#include "BindRequestChunk.hpp"

#include <cstdint>
#include <limits>
#include <string_view>

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    constexpr std::uint32_t PROFILE_VERSION = 1;

    template<typename Integer>
    void append_integer(std::string& buffer, Integer value)
    {
      for (std::size_t i = 0; i < sizeof(value); ++i)
      {
        buffer.push_back(static_cast<char>(value & 0xff));
        value >>= 8;
      }
    }

    void append_size(std::string& buffer, std::size_t size)
    {
      if (size > std::numeric_limits<std::uint32_t>::max())
      {
        throw BindRequestProcessor::Exception("BindRequestChunk: profile is too large");
      }

      append_integer(buffer, static_cast<std::uint32_t>(size));
    }

    template<typename Integer>
    Integer read_integer(std::string_view& buffer)
    {
      if (buffer.size() < sizeof(Integer))
      {
        throw BindRequestProcessor::Exception("BindRequestChunk: truncated profile");
      }

      Integer value = 0;
      for (std::size_t i = 0; i < sizeof(value); ++i)
      {
        value |= static_cast<Integer>(static_cast<unsigned char>(buffer[i])) << (8 * i);
      }

      buffer.remove_prefix(sizeof(value));
      return value;
    }
  }

  BindRequestChunk::BindRequestChunk(
    std::shared_ptr<ProfilingCommons::RocksDBProfileMapProcessor> processor,
    const String::SubString& path,
    const Generics::Time& expire_time)
    : expire_time_(expire_time),
      map_(new RocksDBMap(std::move(processor), path, expire_time, 128,
        Generics::Time::ZERO, false, false))
  {
    if (expire_time_ <= Generics::Time::ZERO)
    {
      throw Exception("BindRequestChunk: expiration time must be positive");
    }

    map_->activate_object();
  }

  BindRequestChunk::~BindRequestChunk() noexcept
  {
    map_->deactivate_object();
    map_->wait_object();
  }

  Commons::StartableAwaitable<void> BindRequestChunk::co_add_bind_request(
    const String::SubString& id,
    const BindRequest& bind_request,
    const Generics::Time& now)
  {
    try
    {
      if (now.tv_sec < 0)
      {
        throw Exception("BindRequestChunk: negative timestamp");
      }

      std::string value;
      append_integer(value, PROFILE_VERSION);
      append_integer(value, static_cast<std::uint64_t>(now.tv_sec));
      append_size(value, bind_request.bind_user_ids.size());
      for (const auto& user_id : bind_request.bind_user_ids)
      {
        append_size(value, user_id.size());
        value.append(user_id);
      }

      const std::string key = id.str();
      Generics::ConstSmartMemBuf_var profile =
        new Generics::ConstSmartMemBuf(value.data(), value.size());
      co_await map_->co_save_profile(key, profile, now);
    }
    catch (const eh::Exception& ex)
    {
      throw Exception(ex.what());
    }
  }

  void BindRequestChunk::add_bind_request(
    const String::SubString& id,
    const BindRequest& bind_request,
    const Generics::Time& now)
  {
    Commons::sync_wait(co_add_bind_request(id, bind_request, now));
  }

  Commons::StartableAwaitable<BindRequestProcessor::BindRequest>
  BindRequestChunk::co_get_bind_request(const String::SubString& id, const Generics::Time& now)
  {
    try
    {
      const std::string key = id.str();
      const auto profile = co_await map_->co_get_profile(key);
      BindRequest result;
      if (!profile)
      {
        co_return result;
      }

      const auto& buffer = profile->membuf();
      std::string_view value(static_cast<const char*>(buffer.data()), buffer.size());
      if (read_integer<std::uint32_t>(value) != PROFILE_VERSION)
      {
        throw Exception("BindRequestChunk: unsupported profile version");
      }

      const auto create_time = read_integer<std::uint64_t>(value);
      if (create_time > static_cast<std::uint64_t>(std::numeric_limits<time_t>::max()))
      {
        throw Exception("BindRequestChunk: invalid profile timestamp");
      }

      const Generics::Time created(static_cast<time_t>(create_time));
      if (now >= created && now - created >= expire_time_)
      {
        co_return result;
      }

      const auto count = read_integer<std::uint32_t>(value);
      if (count > value.size() / sizeof(std::uint32_t))
      {
        throw Exception("BindRequestChunk: invalid user ID count");
      }

      result.bind_user_ids.reserve(count);
      for (std::uint32_t i = 0; i < count; ++i)
      {
        const auto size = read_integer<std::uint32_t>(value);
        if (size > value.size())
        {
          throw Exception("BindRequestChunk: truncated user ID");
        }

        result.bind_user_ids.emplace_back(value.data(), size);
        value.remove_prefix(size);
      }

      if (!value.empty())
      {
        throw Exception("BindRequestChunk: unexpected profile data");
      }

      co_return result;
    }
    catch (const eh::Exception& ex)
    {
      throw Exception(ex.what());
    }
  }

  BindRequestProcessor::BindRequest
  BindRequestChunk::get_bind_request(const String::SubString& id, const Generics::Time& now)
  {
    return Commons::sync_wait(co_get_bind_request(id, now));
  }
}
