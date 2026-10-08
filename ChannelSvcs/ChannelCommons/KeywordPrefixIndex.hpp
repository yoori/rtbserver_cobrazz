#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <string_view>

#include <boost/unordered/unordered_flat_map.hpp>
#include <Generics/Hash.hpp>

namespace AdServer::ChannelSvcs
{
  template<typename Hash>
  class BasicKeywordPrefixIndex
  {
  public:
    struct Counts
    {
      std::size_t terminal = 0;
      std::size_t descendants = 0;
    };

    Counts find(std::size_t hash) const noexcept;

    void add(std::string_view prefix);

    void remove(std::string_view prefix);

  private:
    using Shard = boost::unordered_flat_map<std::size_t, Counts>;
    std::array<std::shared_ptr<Shard>, 256> shards_;

    void update_hash_(std::size_t hash, bool terminal, bool add);

    void update_(std::string_view prefix, bool add);
  };

  using KeywordPrefixIndex = BasicKeywordPrefixIndex<Generics::FastHash>;
}

namespace AdServer::ChannelSvcs
{
  template<typename Hash>
  inline typename BasicKeywordPrefixIndex<Hash>::Counts
  BasicKeywordPrefixIndex<Hash>::find(std::size_t hash) const noexcept
  {
    const auto& shard = shards_[hash % shards_.size()];
    if (shard)
    {
      const auto it = shard->find(hash);
      if (it != shard->end())
      {
        return it->second;
      }
    }
    return {};
  }

  template<typename Hash>
  inline void BasicKeywordPrefixIndex<Hash>::add(std::string_view prefix)
  {
    update_(prefix, true);
  }

  template<typename Hash>
  inline void BasicKeywordPrefixIndex<Hash>::remove(std::string_view prefix)
  {
    update_(prefix, false);
  }

  template<typename Hash>
  inline void BasicKeywordPrefixIndex<Hash>::update_hash_(std::size_t hash, bool terminal, bool add)
  {
    auto& shard = shards_[hash % shards_.size()];
    if (!shard)
    {
      assert(add);
      shard = std::make_shared<Shard>();
    }
    else if (!shard.unique())
    {
      shard = std::make_shared<Shard>(*shard);
    }

    if (add)
    {
      auto& counts = (*shard)[hash];
      ++(terminal ? counts.terminal : counts.descendants);
    }
    else
    {
      const auto it = shard->find(hash);
      assert(it != shard->end());
      auto& counts = it->second;
      auto& count = terminal ? counts.terminal : counts.descendants;
      assert(count != 0);
      --count;
      if (!counts.terminal && !counts.descendants)
      {
        shard->erase(it);
      }
    }
  }

  template<typename Hash>
  inline void BasicKeywordPrefixIndex<Hash>::update_(std::string_view prefix, bool add)
  {
    const auto first_space = prefix.find(' ');
    if (first_space == std::string_view::npos)
    {
      return;
    }

    update_hash_(Hash::hash(prefix.data(), prefix.size()), true, add);
    for (auto pos = prefix.find(' ', first_space + 1); pos != std::string_view::npos;
      pos = prefix.find(' ', pos + 1))
    {
      update_hash_(Hash::hash(prefix.data(), pos), false, add);
    }
  }
}
