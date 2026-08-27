#pragma once

#include <functional>
#include <string>
#include <string_view>

#include <boost/unordered/unordered_flat_map.hpp>

#include <Generics/HashTableAdapters.hpp>
#include <String/SubString.hpp>

namespace AdServer::Bidding::HashMapDetail
{
  inline std::string_view
  to_string_view(const std::string& value) noexcept
  {
    return value;
  }

  inline std::string_view
  to_string_view(std::string_view value) noexcept
  {
    return value;
  }

  inline std::string_view
  to_string_view(const char* value) noexcept
  {
    return value ? std::string_view(value) : std::string_view();
  }

  inline std::string_view
  to_string_view(const String::SubString& value) noexcept
  {
    return std::string_view(value.data(), value.size());
  }

  inline std::string_view
  to_string_view(const Generics::SubStringHashAdapter& value) noexcept
  {
    return std::string_view(value);
  }
}

namespace AdServer::Bidding
{
  struct TransparentStringHash
  {
    using is_transparent = void;

    template<typename Value>
    std::size_t
    operator()(const Value& value) const noexcept
    {
      return std::hash<std::string_view>()(HashMapDetail::to_string_view(value));
    }
  };

  struct TransparentStringEqual
  {
    using is_transparent = void;

    template<typename Left, typename Right>
    bool
    operator()(const Left& left, const Right& right) const noexcept
    {
      return HashMapDetail::to_string_view(left) == HashMapDetail::to_string_view(right);
    }
  };

  struct SubStringHashAdapterHash
  {
    using is_transparent = void;

    std::size_t
    operator()(const Generics::SubStringHashAdapter& value) const noexcept
    {
      return value.hash();
    }

    std::size_t
    operator()(const String::SubString& value) const noexcept
    {
      return Generics::SubStringHashAdapter(value).hash();
    }

    std::size_t
    operator()(std::string_view value) const noexcept
    {
      return operator()(String::SubString(value.data(), value.size()));
    }

    std::size_t
    operator()(const std::string& value) const noexcept
    {
      return operator()(std::string_view(value));
    }
  };

  template<typename Value>
  using StringFlatMap = boost::unordered_flat_map<
    std::string,
    Value,
    TransparentStringHash,
    TransparentStringEqual>;

  template<typename Value>
  using SubStringHashAdapterFlatMap = boost::unordered_flat_map<
    Generics::SubStringHashAdapter,
    Value,
    SubStringHashAdapterHash,
    TransparentStringEqual>;
}
