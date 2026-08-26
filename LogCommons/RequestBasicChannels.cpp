
#include <algorithm>
#include <cstddef>
#include <limits>
#include "RequestBasicChannels.hpp"
#include "BufferWriter.hpp"

#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{
  template <> const char* RequestBasicChannelsTraits::B::base_name_ = "RequestBasicChannels";

  template <> const char* RequestBasicChannelsTraits::B::signature_ = "RequestBasicChannels";

  template <> const char* RequestBasicChannelsTraits::B::current_version_ = "3.4";

  namespace
  {
    const char TRIGGER_MATCH_SEP = ':';

    const char ASI_SEP1 = ':';
    const char ASI_SEP2 = '/';

    const char ABSI_SEP1 = ':';
    const char ABSI_SEP2 = '/';

    const char AD_SELECT_FIELD_SEPARATOR = ':';

    bool
    parse_unsigned_(
      const char*& current,
      const char* end,
      std::uint32_t& value) noexcept
    {
      if (current == end)
      {
        return false;
      }

      if (*current == '+')
      {
        if (++current == end)
        {
          return false;
        }
      }

      constexpr std::uint32_t MAX_VALUE =
        std::numeric_limits<std::uint32_t>::max();
      constexpr std::uint32_t MAX_VALUE_DIV_10 = MAX_VALUE / 10;
      constexpr unsigned int MAX_VALUE_MOD_10 = MAX_VALUE % 10;

      const char* const begin = current;
      constexpr std::ptrdiff_t SAFE_DIGITS =
        std::numeric_limits<std::uint32_t>::digits10;
      const char* const safe_end = end - current < SAFE_DIGITS ?
        end : current + SAFE_DIGITS;

      std::uint32_t result = 0;
      while (current != safe_end)
      {
        const unsigned int digit =
          static_cast<unsigned char>(*current) -
          static_cast<unsigned char>('0');
        if (digit > 9)
        {
          break;
        }

        result = result * 10 + digit;
        ++current;
      }

      if (current == safe_end && current != end)
      {
        unsigned int digit =
          static_cast<unsigned char>(*current) -
          static_cast<unsigned char>('0');
        while (digit <= 9)
        {
          if (result > MAX_VALUE_DIV_10 ||
            (result == MAX_VALUE_DIV_10 && digit > MAX_VALUE_MOD_10))
          {
            return false;
          }

          result = result * 10 + digit;
          ++current;
          if (current == end)
          {
            break;
          }

          digit =
            static_cast<unsigned char>(*current) -
            static_cast<unsigned char>('0');
        }
      }

      if (current == begin)
      {
        return false;
      }

      value = result;
      return true;
    }

    bool
    parse_trigger_matches_(
      const String::SubString& token,
      RequestBasicChannelsInnerData::TriggerMatchArray& values)
    {
      constexpr std::size_t EXPECTED_TRIGGER_MATCH_SIZE = 17;

      RequestBasicChannelsInnerData::TriggerMatchArray result;
      result.reserve(token.size() / EXPECTED_TRIGGER_MATCH_SIZE + 1);

      const char* current = token.begin();
      const char* const end = token.end();
      while (current != end)
      {
        std::uint32_t channel_id = 0;
        std::uint32_t channel_trigger_id = 0;
        if (!parse_unsigned_(current, end, channel_id) ||
          current == end || *current != ':')
        {
          return false;
        }

        ++current;
        if (!parse_unsigned_(current, end, channel_trigger_id))
        {
          return false;
        }

        result.emplace_back(channel_id, channel_trigger_id);
        if (current == end)
        {
          break;
        }

        if (*current != ',' || ++current == end)
        {
          return false;
        }
      }

      values.swap(result);
      return true;
    }
  } // namespace

  std::istream&
  operator>>(std::istream& is, RequestBasicChannelsKey& key)
  {
    is >> key.time_;
    read_eol(is);
    is >> key.isp_time_;
    read_eol(is);
    is >> key.colo_id_;
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const RequestBasicChannelsKey& key)
    /*throw(eh::Exception)*/
  {
    os << key.time_ << '\n' << key.isp_time_ << '\n' << key.colo_id_;
    return os;
  }

  FixedBufStream<SlashCategory>&
  operator>>(
    FixedBufStream<SlashCategory>& is,
    RequestBasicChannelsInnerData::AdBidSlotImpression& value)
  {
    const String::SubString token = is.read_token();
    if (is.good())
    {
      FixedBufStream<SemiCategory> stream(token);
      value.data_ = new RequestBasicChannelsInnerData::AdBidSlotImpression::Data();
      stream >> value.data_->revenue;
      stream >> value.data_->revenue_bid;
      stream >> value.data_->impression_channels;
      is.take_fails(stream);
    }
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(
    FixedBufStream<TabCategory>& is,
    RequestBasicChannelsInnerData::AdBidSlotImpressionList& values)
  {
    const String::SubString token = is.read_token();
    if (is.good())
    {
      if (token == "-")
      {
        values.clear();
        return is;
      }
      typedef RequestBasicChannelsInnerData::AdBidSlotImpressionList List;
      List container;
      FixedBufStream<SlashCategory> list_stream(token);
      while (true)
      {
        List::value_type elem;
        list_stream >> elem;
        if (!list_stream.good())
        {
          break;
        }
        container.push_back(elem);
      }

      is.take_fails(list_stream);
      if (is.good())
      {
        values.swap(container);
      }
    }
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(
    FixedBufStream<TabCategory>& is,
    RequestBasicChannelsInnerData::TriggerMatchArray& values)
  {
    const String::SubString token = is.read_token();
    if (is.good())
    {
      if (token == "-")
      {
        values.clear();
        return is;
      }

      if (!parse_trigger_matches_(token, values))
      {
        is.setstate(std::ios_base::failbit);
      }
    }
    return is;
  }

  using namespace CampaignSvcs;

  FixedBufStream<TabCategory>&
  operator>>(
    FixedBufStream<TabCategory>& is,
    RequestBasicChannelsInnerData::AdRequestProps& value)
  {
    // inplace loading from stream
    value.data_ = new RequestBasicChannelsInnerData::AdRequestProps::Data;
    is >> value.data_->sizes;
    assert(!is.fail());
    is >> value.data_->country_code;
    assert(!is.fail());
    is >> value.data_->max_text_ads;
    assert(!is.fail());
    is >> value.data_->text_ad_cost_threshold;
    assert(!is.fail());
    is >> value.data_->display_ad_shown;
    assert(!is.fail());
    is >> value.data_->text_ad_shown;
    assert(!is.fail());
    is >> value.data_->ad_select;
    assert(!is.fail());
    value.data_->auction_type = get_auction_type(is);
    value.normalize();
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(
    FixedBufStream<TabCategory>& is,
    RequestBasicChannelsInnerData::AdSelectProps& value)
    /*throw(eh::Exception)*/
  {
    const String::SubString token = is.read_token();
    if (is.good())
    {
      FixedBufStream<SemiCategory> stream(token);
      value.data_ =
        new RequestBasicChannelsInnerData::AdSelectProps::Data();
      stream >> value.data_->tag_id;
      stream >> value.data_->size;
      stream >> value.data_->format;
      stream >> value.data_->test_request;
      stream >> value.data_->profiling_available;
      stream >> value.data_->full_freq_caps;
      is.take_fails(stream);
    }
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(
    FixedBufStream<TabCategory>& is,
    RequestBasicChannelsInnerData::AdSlotImpression& value)
  {
    const String::SubString token = is.pop_token();
    if (token.empty())
    {
      is.setstate(std::ios_base::eofbit);
    }
    else
    {
      FixedBufStream<SemiCategory> stream(token);
      value.data_ = new RequestBasicChannelsInnerData::AdSlotImpression::Data;
      stream >> value.data_->revenue;
      stream >> value.data_->impression_channels;
      is.take_fails(stream);
    }
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(
    FixedBufStream<TabCategory>& is,
    RequestBasicChannelsInnerData::Match& match_request)
  {
    // inplace loading from stream
    match_request.data_ = new RequestBasicChannelsInnerData::Match::Data;
    is >> match_request.data_->history_channels;
    is >> match_request.data_->page_trigger_channels;
    is >> match_request.data_->search_trigger_channels;
    is >> match_request.data_->url_trigger_channels;
    is >> match_request.data_->url_keyword_trigger_channels;
    return is;
  }

  std::istream&
  operator>>(std::istream& is, RequestBasicChannelsInnerData& data)
    /*throw(eh::Exception)*/
  {
    data.holder_ = new RequestBasicChannelsInnerData::DataHolder();
    thread_local std::string record;
    record.clear();
    if (record.capacity() < 1024)
    {
      record.reserve(1024);
    }
    read_until_eol(is, record);
    if (is.good())
    {
      FixedBufStream<TabCategory> fixed_buf_stream(record);
      TokenizerInputArchive<> ia(fixed_buf_stream);
      ia >> *data.holder_;
      fixed_buf_stream.transfer_state(is);
    }
    return is;
  }

  std::ostream&
  operator<<(
    std::ostream& os,
    const RequestBasicChannelsInnerData::TriggerMatch& match)
  {
    os << match.channel_id << TRIGGER_MATCH_SEP;
    os << match.channel_trigger_id;
    return os;
  }

  BufferWriter&
  operator<<(
    BufferWriter& out,
    const RequestBasicChannelsInnerData::TriggerMatch& match)
  {
    out << match.channel_id << TRIGGER_MATCH_SEP << match.channel_trigger_id;
    return out;
  }

  std::ostream&
  operator<<(
    std::ostream& os,
    const RequestBasicChannelsInnerData::TriggerMatchArray& trigger_matches)
  {
    if (!trigger_matches.empty())
    {
      char SEP[2] = { ',' };
      output_sequence(os, trigger_matches, SEP);
    }
    else
    {
      os << '-';
    }
    return os;
  }

  BufferWriter&
  operator<<(
    BufferWriter& out,
    const RequestBasicChannelsInnerData::TriggerMatchArray& trigger_matches)
  {
    return write_sequence_(out, trigger_matches, ',');
  }

  std::ostream&
  operator<<(
    std::ostream& os,
    const RequestBasicChannelsInnerData::AdSlotImpression& ad_imp)
  {
    os << ad_imp.data_->revenue << ASI_SEP1;
    os << ad_imp.data_->impression_channels;
    return os;
  }

  BufferWriter&
  operator<<(
    BufferWriter& out,
    const RequestBasicChannelsInnerData::AdSlotImpression& ad_imp)
  {
    out << ad_imp.revenue() << ASI_SEP1
      << ad_imp.impression_channels();
    return out;
  }

  std::ostream&
  operator<<(
    std::ostream& os,
    const RequestBasicChannelsInnerData::AdBidSlotImpression& absi)
  {
    os << absi.data_->revenue << ABSI_SEP1;
    os << absi.data_->revenue_bid << ABSI_SEP1;
    os << absi.data_->impression_channels;
    return os;
  }

  BufferWriter&
  operator<<(
    BufferWriter& out,
    const RequestBasicChannelsInnerData::AdBidSlotImpression& absi)
  {
    out << absi.revenue() << ABSI_SEP1
      << absi.revenue_bid() << ABSI_SEP1
      << absi.impression_channels();
    return out;
  }

  std::ostream&
  operator<<(
    std::ostream& os,
    const RequestBasicChannelsInnerData::AdBidSlotImpressionList& absi_list)
  {
    if (!absi_list.empty())
    {
      char SEP[2] = { ABSI_SEP2 };
      output_sequence(os, absi_list, SEP);
    }
    else
    {
      os << '-';
    }
    return os;
  }

  BufferWriter&
  operator<<(
    BufferWriter& out,
    const RequestBasicChannelsInnerData::AdBidSlotImpressionList& absi_list)
  {
    return write_sequence_(out, absi_list, ABSI_SEP2);
  }

  std::ostream&
  operator<<(
    std::ostream& os,
    const RequestBasicChannelsInnerData::AdRequestProps& ad_req)
  {
    os << ad_req.data_->sizes << '\t';
    os << ad_req.data_->country_code << '\t';
    os << ad_req.data_->max_text_ads << '\t';
    os << ad_req.data_->text_ad_cost_threshold << '\t';
    os << ad_req.data_->display_ad_shown << '\t';
    os << ad_req.data_->text_ad_shown << '\t';
    os << ad_req.data_->ad_select << '\t';
    os << put_auction_type(ad_req.data_->auction_type);
    return os;
  }

  BufferWriter&
  operator<<(
    BufferWriter& out,
    const RequestBasicChannelsInnerData::AdRequestProps& ad_req)
  {
    out << ad_req.sizes() << '\t'
      << StringIoWrapperOptional(ad_req.country_code()) << '\t'
      << ad_req.max_text_ads() << '\t'
      << ad_req.text_ad_cost_threshold() << '\t'
      << ad_req.display_ad_shown() << '\t'
      << ad_req.text_ad_shown() << '\t'
      << ad_req.ad_select() << '\t'
      << put_auction_type(ad_req.auction_type());
    return out;
  }

  std::ostream&
  operator<<(
    std::ostream& os,
    const RequestBasicChannelsInnerData::AdSelectProps& ad_select)
    /*throw(eh::Exception)*/
  {
    os << ad_select.data_->tag_id << AD_SELECT_FIELD_SEPARATOR;
    os << ad_select.data_->size << AD_SELECT_FIELD_SEPARATOR;
    os << ad_select.data_->format << AD_SELECT_FIELD_SEPARATOR;
    os << ad_select.data_->test_request << AD_SELECT_FIELD_SEPARATOR;
    os << ad_select.data_->profiling_available << AD_SELECT_FIELD_SEPARATOR;
    os << ad_select.data_->full_freq_caps;
    return os;
  }

  BufferWriter&
  operator<<(
    BufferWriter& out,
    const RequestBasicChannelsInnerData::AdSelectProps& ad_select
  )
  {
    out << ad_select.tag_id() << AD_SELECT_FIELD_SEPARATOR
      << SpacesMarksString(ad_select.size()) << AD_SELECT_FIELD_SEPARATOR
      << SpacesMarksString(ad_select.format()) << AD_SELECT_FIELD_SEPARATOR
      << ad_select.test_request() << AD_SELECT_FIELD_SEPARATOR
      << ad_select.profiling_available() << AD_SELECT_FIELD_SEPARATOR
      << ad_select.full_freq_caps();
    return out;
  }

  std::ostream&
  operator<<(
    std::ostream& os,
    const RequestBasicChannelsInnerData::Match& match_request)
  {
    os << match_request.data_->history_channels << '\t';
    os << match_request.data_->page_trigger_channels << '\t';
    os << match_request.data_->search_trigger_channels << '\t';
    os << match_request.data_->url_trigger_channels << '\t';
    os << match_request.data_->url_keyword_trigger_channels;
    return os;
  }

  BufferWriter&
  operator<<(
    BufferWriter& out,
    const RequestBasicChannelsInnerData::Match& match_request
  )
  {
    out << match_request.history_channels() << '\t'
      << match_request.page_trigger_channels() << '\t'
      << match_request.search_trigger_channels() << '\t'
      << match_request.url_trigger_channels() << '\t'
      << match_request.url_keyword_trigger_channels();
    return out;
  }

  std::ostream&
  operator<<(std::ostream& os, const RequestBasicChannelsInnerData& data)
    /*throw(eh::Exception)*/
  {
    BufferWriter writer(512);
    writer << data;
    writer.write_to(os);
    return os;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestBasicChannelsInnerData& data)
    /*throw(eh::Exception)*/
  {
    data.holder_->invariant();
    out << data.holder_->user_type << '\t'
      << data.holder_->user_id << '\t'
      << data.holder_->temporary_user_id << '\t'
      << data.holder_->match_request << '\t'
      << data.holder_->ad_request;
    return out;
  }
} // namespace AdServer::LogProcessing
