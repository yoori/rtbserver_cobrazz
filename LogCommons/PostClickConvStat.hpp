#pragma once

#include "PostClickStat.hpp"

namespace AdServer::LogProcessing
{
  class PostClickConvStatInnerKey: public PostClickStatInnerKey
  {
  public:
    PostClickConvStatInnerKey() noexcept;
    PostClickConvStatInnerKey(
      CreativeStatInnerKey creative_key, std::uint64_t connection_type_id,
      const std::string& conversion_name);

    bool operator==(const PostClickConvStatInnerKey& rhs) const noexcept;

    const std::string& conversion_name() const noexcept;
    std::size_t hash() const noexcept;
    void invariant() const;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, PostClickConvStatInnerKey& key);
    friend BufferWriter& operator<<(BufferWriter& out, const PostClickConvStatInnerKey& key);

  private:
    void calc_hash_() noexcept;
    Aux_::StringIoWrapper conversion_name_;
    std::size_t hash_ = 0;
  };

  class PostClickConvStatInnerData
  {
  public:
    PostClickConvStatInnerData() noexcept;
    PostClickConvStatInnerData(std::uint64_t visits, std::uint64_t goals) noexcept;

    bool operator==(const PostClickConvStatInnerData& rhs) const noexcept;

    PostClickConvStatInnerData& operator+=(const PostClickConvStatInnerData& rhs) noexcept;

    std::uint64_t visits() const noexcept;
    std::uint64_t goals() const noexcept;
    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, PostClickConvStatInnerData& data);
    friend BufferWriter& operator<<(BufferWriter& out, const PostClickConvStatInnerData& data);

  private:
    std::uint64_t visits_ = 0;
    std::uint64_t goals_ = 0;
  };

  class PostClickConvStatInnerKey_V_1_0: public PostClickStatInnerKey_V_1_1
  {
  public:
    bool operator==(const PostClickConvStatInnerKey_V_1_0& rhs) const noexcept;
    PostClickConvStatInnerKey current_key() const;
    std::size_t hash() const noexcept;
    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, PostClickConvStatInnerKey_V_1_0& key);

  private:
    Aux_::StringIoWrapper conversion_name_;
    std::size_t hash_ = 0;
  };

  using PostClickConvStatInnerCollector =
    StatCollector<PostClickConvStatInnerKey, PostClickConvStatInnerData, false, true>;
  using PostClickConvStatCollector =
    StatCollector<CreativeStatKey, PostClickConvStatInnerCollector>;
  using PostClickConvStatInnerCollector_V_1_0 =
    StatCollector<PostClickConvStatInnerKey_V_1_0, PostClickConvStatInnerData, false, true>;
  using PostClickConvStatCollector_V_1_0 =
    StatCollector<CreativeStatKey, PostClickConvStatInnerCollector_V_1_0>;

  struct PostClickConvStatTraits: LogDefaultTraits<PostClickConvStatCollector>
  {
    static CollectorType convert_collector(const PostClickConvStatCollector_V_1_0& old_collector);

    template <class Functor>
    static void for_each_old(Functor& functor);
  };

  template <class Functor>
  void PostClickConvStatTraits::for_each_old(Functor& functor)
  {
    functor.template support<PostClickConvStatCollector_V_1_0, true>("1.0");
  }
}
