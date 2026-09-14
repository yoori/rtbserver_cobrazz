#include "PostImpStat.hpp"
#include "BufferWriter.hpp"
namespace AdServer::LogProcessing
{
  template <> const char* PostImpStatTraits::B::base_name_ = "PostImpStat";
  template <> const char* PostImpStatTraits::B::signature_ = "PostImpStat";
  template <> const char* PostImpStatTraits::B::current_version_ = "1.0";
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostImpStatInnerData& data)
  {
    for (unsigned i = 0; i < 13; ++i)
    {
      is >> data.values_[i];
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const PostImpStatInnerData& data)
  {
    for (unsigned i = 0; i < 13; ++i)
    {
      if (i)
      {
        out << '\t';
      }
      out << data.values_[i];
    }
    return out;
  }
}
