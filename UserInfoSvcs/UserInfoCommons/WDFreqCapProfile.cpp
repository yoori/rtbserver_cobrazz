#include <iostream>

#include <Generics/Time.hpp>
#include <Commons/Algs.hpp>

#include "WDFreqCapProfile.hpp"

namespace
{
  const unsigned long CURRENT_VERSION = 1;
}

namespace
{
  struct UIntToTime
  {
    Generics::Time operator()(unsigned long val) const
    {
      return Generics::Time(val);
    }
  };

  struct TimeToUInt
  {
    uint32_t operator()(uint32_t val) const
    {
      return val;
    }

    uint32_t operator()(
      const Generics::Time& val) const
    {
      return val.tv_sec;
    }
  };

  struct TimeUIntCmp
  {
    bool operator()(const Generics::Time& left,
      uint32_t right) const
    {
      return static_cast<unsigned long>(left.tv_sec) < right;
    }

    bool operator()(uint32_t left,
      const Generics::Time& right) const
    {
      return left < static_cast<unsigned long>(right.tv_sec);
    }
  };
}

namespace
{
  using namespace AdServer::UserInfoSvcs;
  
  template<typename PlainReaderType, typename IdType>
  struct CommonIdImpressionCmp
  {
    bool operator()(const PlainReaderType& left,
      const IdType& right) const
    {
      return right > left.id();
    }

    bool operator()(
      const IdType& left,
      const PlainReaderType& right) const
    {
      return left < right.id();
    }
  };

  typedef CommonIdImpressionCmp<
    PlainIdImpressionsReader,
    unsigned long>
    IdImpressionCmp;

  typedef CommonIdImpressionCmp<
    PlainStringIdImpressionsReader,
    std::string>
    StringIdImpressionCmp;

  template<typename ReturnType, typename LeftType, typename RightType>
  class CommonIdImpressionMerge
  {
  public:
    CommonIdImpressionMerge(const Generics::Time& time)
      : time_(time)
    {}

    ReturnType operator()(const LeftType& left, const RightType& /*right*/) const
    {
      // right type is id here (string or unsigned long)
      ReturnType ret;
      ret.id() = left.id();
      ret.total() = left.total() + 1;

      // insert time impression into result sequence (without uniq)
      std::merge(
        left.times().begin(), left.times().end(),
        &time_, &time_ + 1,
        Algs::modify_inserter(
          std::back_inserter(ret.times()), TimeToUInt()),
        TimeUIntCmp());

      return ret;
    }

  private:
    Generics::Time time_;
  };

  typedef CommonIdImpressionMerge<
    PlainIdImpressionsWriter,
    PlainIdImpressionsReader,
    unsigned long>
    IdImpressionMerge;

  typedef CommonIdImpressionMerge<
    PlainStringIdImpressionsWriter,
    PlainStringIdImpressionsReader,
    std::string>
    StringIdImpressionMerge;

  template<
    typename PlainReaderType,
    typename PlainWriterType,
    typename IdType>
  struct CommonIdToPlainWriter
  {
    CommonIdToPlainWriter(
      const Generics::Time& time,
      const AdServer::Commons::FreqCap& freq_cap)
      : time_(time),
        freq_cap_(freq_cap),
        check_time_(time - freq_cap.window_time)
    {}

    PlainWriterType
    operator()(const PlainReaderType& val) const
    {
      PlainWriterType ret(val);
      filter_impressions_(ret);
      return ret;
    }

    PlainWriterType
    operator()(const PlainWriterType& val) const
    {
      PlainWriterType ret(val);
      filter_impressions_(ret);
      return ret;
    }

    PlainWriterType operator()(const IdType& val) const
    {
      PlainWriterType ret;
      ret.id() = val;
      ret.total() = 1;

      if((freq_cap_.window_limit != 0 &&
         freq_cap_.window_time != Generics::Time::ZERO) ||
         freq_cap_.period != Generics::Time::ZERO)
      {
        ret.times().push_back(time_.tv_sec);
      }

      return ret;
    }

  private:
    void filter_impressions_(PlainWriterType& val) const
    {
      typename PlainWriterType::times_Container::
        reverse_iterator erase_it = val.times().rbegin();

      unsigned long count = 0;

      if(freq_cap_.period != Generics::Time::ZERO &&
         erase_it != val.times().rend() &&
         Generics::Time(*erase_it) > time_ - freq_cap_.period)
      {
        ++count;
        ++erase_it;
      }

      if(freq_cap_.window_time != Generics::Time::ZERO)
      {
        for(; erase_it != val.times().rend(); ++erase_it, ++count)
        {
          if(Generics::Time(*erase_it) < check_time_ ||
             count >= freq_cap_.window_limit)
          {
            break;
          }
        }
      }

      val.times().erase(val.times().begin(), erase_it.base());
    }

  private:
    const Generics::Time time_;
    const AdServer::Commons::FreqCap freq_cap_;
    const Generics::Time check_time_;
  };

  typedef CommonIdToPlainWriter<
    PlainStringIdImpressionsReader,
    PlainStringIdImpressionsWriter,
    std::string>
    StringIdToPlainWriter;

  typedef CommonIdToPlainWriter<
    PlainIdImpressionsReader,
    PlainIdImpressionsWriter,
    unsigned long>
    IdToPlainWriter;

  class ImpressionsFilter
  {
  public:
    ImpressionsFilter(
      const AdServer::Commons::FreqCap& freq_cap)
      : freq_cap_(freq_cap)
    {}

    template<typename PlainWriterType>
    bool operator()(const PlainWriterType& val) const
    {
      return freq_cap_.lifelimit != 0 ||
        !val.times().empty();
    }

  private:
    AdServer::Commons::FreqCap freq_cap_;
  };

  template<typename IteratorType>
  std::ostream&
  print_times(std::ostream& out,
    IteratorType it, IteratorType it_end)
  {
    for(; it != it_end; ++it)
    {
      out << " " << Generics::Time(*it).get_gm_time();
    }
    return out;
  }

  template<typename IteratorType>
  std::ostream&
  print_imps(std::ostream& out,
    IteratorType it, IteratorType it_end, const char* prefix)
  {
    for(; it != it_end; ++it)
    {
      out << prefix << (*it).id() << " (total = " << (*it).total()
        << ") (hold = " << (*it).times().size() << "):";
      print_times(out, (*it).times().begin(), (*it).times().end());
      out << std::endl;
    }
    return out;
  }
}

namespace AdServer
{
namespace UserInfoSvcs
{
  template<typename ResultType>
  struct PlainImpressionConvert
  {
    template<typename InputType>
    ResultType operator()(const InputType& in) const
    {
      ResultType res;
      res.id = in.id();
      res.total = in.total();
      std::copy(in.times().begin(), in.times().end(),
        Algs::modify_inserter(std::back_inserter(res.times), UIntToTime()));
      return res;
    }
  };

  WDFreqCapProfile::WDFreqCapProfile(SmartMemBufPtr plain_profile)
    : plain_profile_(ReferenceCounting::add_ref(plain_profile))
  {}

  void WDFreqCapProfile::get_impressions(
    NewsItemImpressionList* news_ids,
    IdImpressionList* category_ids,
    IdImpressionList* channel_ids) const
  {
    if(plain_profile_->membuf().size() != 0)
    {
      PlainProfileReader plain_reader(
        plain_profile_->membuf().data(), plain_profile_->membuf().size());

      if(news_ids)
      {
        std::copy(
          plain_reader.news_item_impressions().begin(),
          plain_reader.news_item_impressions().end(),
          Algs::modify_inserter(
            std::back_inserter(*news_ids),
            PlainImpressionConvert<NewsItemImpression>()));
      }

      if(category_ids)
      {
        std::copy(
          plain_reader.category_impressions().begin(),
          plain_reader.category_impressions().end(),
          Algs::modify_inserter(
            std::back_inserter(*category_ids),
            PlainImpressionConvert<IdImpression>()));
      }

      if(channel_ids)
      {
        std::copy(
          plain_reader.channel_impressions().begin(),
          plain_reader.channel_impressions().end(),
          Algs::modify_inserter(
            std::back_inserter(*channel_ids),
            PlainImpressionConvert<IdImpression>()));
      }
    }
  }

  void WDFreqCapProfile::fill_impressions(
    const Generics::Time& time,
    const AdServer::Commons::FreqCap& event_fc,
    const AdServer::Commons::FreqCap& category_fc,
    const AdServer::Commons::FreqCap& channel_fc,
    const NewsIdSet& news_imps,
    const IdSet& category_imps,
    const IdSet& channel_imps)
  {
    if(plain_profile_->membuf().size() == 0)
    {
      PlainProfileWriter plain_writer;
      Generics::MemBuf mb(plain_writer.size());
      plain_profile_->membuf().assign(mb.data(), mb.size());
      plain_writer.save(
        plain_profile_->membuf().data(),
        plain_profile_->membuf().size());
    }

    PlainProfileReader plain_reader(
      plain_profile_->membuf().data(), plain_profile_->membuf().size());

    PlainProfileWriter plain_writer;

    plain_writer.version() = CURRENT_VERSION;

    Algs::merge_unique(
      plain_reader.news_item_impressions().begin(),
      plain_reader.news_item_impressions().end(),
      news_imps.begin(),
      news_imps.end(),
      Algs::modify_inserter(
        Algs::filter_inserter(
          std::back_inserter(plain_writer.news_item_impressions()),
          ImpressionsFilter(event_fc)),
        StringIdToPlainWriter(time, event_fc)),
      StringIdImpressionCmp(),
      StringIdImpressionMerge(time));

    Algs::merge_unique(
      plain_reader.category_impressions().begin(),
      plain_reader.category_impressions().end(),
      category_imps.begin(),
      category_imps.end(),
      Algs::modify_inserter(
        Algs::filter_inserter(
          std::back_inserter(plain_writer.category_impressions()),
          ImpressionsFilter(category_fc)),
        IdToPlainWriter(time, category_fc)),
      IdImpressionCmp(),
      IdImpressionMerge(time));

    Algs::merge_unique(
      plain_reader.channel_impressions().begin(),
      plain_reader.channel_impressions().end(),
      channel_imps.begin(),
      channel_imps.end(),
      Algs::modify_inserter(
        Algs::filter_inserter(
          std::back_inserter(plain_writer.channel_impressions()),
          ImpressionsFilter(channel_fc)),
        IdToPlainWriter(time, channel_fc)),
      IdImpressionCmp(),
      IdImpressionMerge(time));

    Generics::MemBuf mb(plain_writer.size());
    plain_profile_->membuf().assign(mb.data(), mb.size());
    plain_writer.save(
      plain_profile_->membuf().data(),
      plain_profile_->membuf().size());
  }

  std::ostream&
  WDFreqCapProfile::print(std::ostream& out, const char* prefix) const
  {
    return print(
      out,
      plain_profile_->membuf().data(),
      plain_profile_->membuf().size(),
      prefix);
  }

  std::ostream& WDFreqCapProfile::print(
    std::ostream& out,
    const void* buf,
    unsigned long buf_size,
    const char* prefix)
  {
    if(buf_size == 0)
    {
      out << std::endl << prefix << "  Profile is empty." << std::endl;
    }
    else
    {
      std::string sub_prefix(prefix);
      sub_prefix += "  ";
      PlainProfileReader plain_reader(buf, buf_size);
      out << prefix << "news impressions: " << std::endl;
      print_imps(out,
        plain_reader.news_item_impressions().begin(),
        plain_reader.news_item_impressions().end(),
        sub_prefix.c_str());
      out << prefix << "category impressions: " << std::endl;
      print_imps(out,
        plain_reader.category_impressions().begin(),
        plain_reader.category_impressions().end(),
        sub_prefix.c_str());
      out << prefix << "channel impressions: " << std::endl;
      print_imps(out,
        plain_reader.channel_impressions().begin(),
        plain_reader.channel_impressions().end(),
                 sub_prefix.c_str());
    }

    return out;
  }
}
}
