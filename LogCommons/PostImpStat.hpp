#pragma once

#include <cstdint>
#include "CreativeStat.hpp"
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer::LogProcessing
{
  class PostImpStatInnerData
  {
  public:
    PostImpStatInnerData() noexcept : values_() {}

    explicit PostImpStatInnerData(unsigned index) noexcept : values_()
    {
      if (index < 13)
      {
        values_[index] = 1;
      }
    }

    PostImpStatInnerData&
    operator+=(const PostImpStatInnerData& rhs) noexcept
    {
      for (unsigned i = 0; i < 13; ++i)
      {
        values_[i] += rhs.values_[i];
      }
      return *this;
    }

    std::uint64_t
    value(unsigned index) const noexcept
    {
      return index < 13 ? values_[index] : 0;
    }

    bool
    operator==(const PostImpStatInnerData& rhs) const noexcept
    {
      for (unsigned i = 0; i < 13; ++i)
      {
        if (values_[i] != rhs.values_[i])
        {
          return false;
        }
      }
      return true;
    }
    friend FixedBufStream<TabCategory>& operator>>(FixedBufStream<TabCategory>&, PostImpStatInnerData&);
    friend BufferWriter& operator<<(BufferWriter&, const PostImpStatInnerData&);
  private:
    std::uint64_t values_[13];
  };
  using PostImpStatCollector = StatCollector<CreativeStatKey,
    StatCollector<CreativeStatInnerKey, PostImpStatInnerData, false, true>>;
  struct PostImpStatTraits: LogDefaultTraits<PostImpStatCollector> {};
}
