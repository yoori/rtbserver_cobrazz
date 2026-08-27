
#include <algorithm>
#include <string_view>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

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

    const char*
    find_separator_(const char* current, const char* end, char separator) noexcept
    {
#if defined(__SSE2__)
      const __m128i expected = _mm_set1_epi8(separator);
      while (end - current >= 16)
      {
        const __m128i bytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(current));
        const unsigned int mask = static_cast<unsigned int>(_mm_movemask_epi8(
          _mm_cmpeq_epi8(bytes, expected)));
        if (mask != 0)
        {
          return current + __builtin_ctz(mask);
        }

        current += 16;
      }
#endif

      return std::find(current, end, separator);
    }

    bool
    read_component_(
      const char*& current,
      const char* end,
      char separator,
      std::string_view& component) noexcept
    {
      if (current == end)
      {
        return false;
      }

      const char* const begin = current;
      current = find_separator_(current, end, separator);
      if (current == begin)
      {
        return false;
      }

      component = std::string_view(begin, current - begin);
      if (current != end)
      {
        if (++current == end)
        {
          return false;
        }
      }

      return true;
    }

    bool
    parse_uint32_list_(std::string_view token, NumberArray& values)
    {
      if (token == "-")
      {
        values.clear();
        return true;
      }

      constexpr std::size_t EXPECTED_VALUE_SIZE = 9;
      NumberArray result;
      result.reserve(token.size() / EXPECTED_VALUE_SIZE + 1);

      const char* current = token.begin();
      const char* const end = token.end();
      while (current != end)
      {
        const char* const separator = find_separator_(current, end, ',');
        const std::string_view value_token(current, separator - current);
        std::uint32_t value = 0;
        if (!String::StringManip::str_to_int(value_token, value))
        {
          return false;
        }
        result.push_back(value);

        current = separator;
        if (current != end && ++current == end)
        {
          return false;
        }
      }

      values.swap(result);
      return true;
    }

    bool
    parse_fixed_number_(std::string_view token, RequestBasicChannelsInnerData::FixedNum& value)
    {
      if (token.empty())
      {
        return false;
      }

      try
      {
        value = RequestBasicChannelsInnerData::FixedNum(token);
        return true;
      }
      catch (const eh::Exception&)
      {
        return false;
      }
    }

    bool
    parse_bool_(std::string_view token, bool& value) noexcept
    {
      if (token == "0")
      {
        value = false;
        return true;
      }

      if (token == "1")
      {
        value = true;
        return true;
      }

      return false;
    }

    template <typename StringType>
    bool
    parse_escaped_string_(std::string_view token, StringType& value)
    {
      if (token.empty())
      {
        return false;
      }

      StringType result;
      result.reserve(token.size());
      const char* const end = token.end();
      for (const char* current = token.begin(); current != end; ++current)
      {
        char ch = *current;
        if (ch == Aux_::EscapeChar::ESC_CHAR)
        {
          unsigned int decoded = 0;
          for (std::size_t index = 0; index < 2; ++index)
          {
            if (++current == end)
            {
              return false;
            }

            unsigned int digit = *current - '0';
            if (digit > 9)
            {
              digit -= 7;
              if (digit < 10 || digit > 15)
              {
                return false;
              }
            }
            decoded = decoded * 16 + digit;
          }
          ch = static_cast<char>(decoded);
        }
        result.push_back(ch);
      }

      value.swap(result);
      return true;
    }

    bool
    parse_ad_slot_impression_(
      std::string_view token,
      RequestBasicChannelsInnerData::AdSlotImpression& value)
    {
      const char* current = token.begin();
      const char* const end = token.end();
      std::string_view revenue_token;
      std::string_view channels_token;
      if (!read_component_(current, end, ASI_SEP1, revenue_token) ||
        !read_component_(current, end, ASI_SEP1, channels_token) ||
        current != end)
      {
        return false;
      }

      RequestBasicChannelsInnerData::FixedNum revenue;
      NumberArray impression_channels;
      if (!parse_fixed_number_(revenue_token, revenue) ||
        !parse_uint32_list_(channels_token, impression_channels))
      {
        return false;
      }

      value = RequestBasicChannelsInnerData::AdSlotImpression(revenue, impression_channels);
      return true;
    }

    bool
    parse_ad_bid_slot_impression_(
      std::string_view token,
      RequestBasicChannelsInnerData::AdBidSlotImpression& value)
    {
      const char* current = token.begin();
      const char* const end = token.end();
      std::string_view revenue_token;
      std::string_view revenue_bid_token;
      std::string_view channels_token;
      if (!read_component_(current, end, ABSI_SEP1, revenue_token) ||
        !read_component_(current, end, ABSI_SEP1, revenue_bid_token) ||
        !read_component_(current, end, ABSI_SEP1, channels_token) ||
        current != end)
      {
        return false;
      }

      RequestBasicChannelsInnerData::FixedNum revenue;
      RequestBasicChannelsInnerData::FixedNum revenue_bid;
      NumberArray impression_channels;
      if (!parse_fixed_number_(revenue_token, revenue) ||
        !parse_fixed_number_(revenue_bid_token, revenue_bid) ||
        !parse_uint32_list_(channels_token, impression_channels))
      {
        return false;
      }

      value = RequestBasicChannelsInnerData::AdBidSlotImpression(
        revenue,
        revenue_bid,
        impression_channels);
      return true;
    }

    bool
    parse_ad_select_props_(
      std::string_view token,
      RequestBasicChannelsInnerData::AdSelectProps& value)
    {
      const char* current = token.begin();
      const char* const end = token.end();
      std::string_view tag_id_token;
      std::string_view size_token;
      std::string_view format_token;
      std::string_view test_request_token;
      std::string_view profiling_available_token;
      std::string_view full_freq_caps_token;
      if (!read_component_(current, end, AD_SELECT_FIELD_SEPARATOR, tag_id_token) ||
        !read_component_(current, end, AD_SELECT_FIELD_SEPARATOR, size_token) ||
        !read_component_(current, end, AD_SELECT_FIELD_SEPARATOR, format_token) ||
        !read_component_(current, end, AD_SELECT_FIELD_SEPARATOR, test_request_token) ||
        !read_component_(current, end, AD_SELECT_FIELD_SEPARATOR, profiling_available_token) ||
        !read_component_(current, end, AD_SELECT_FIELD_SEPARATOR, full_freq_caps_token) ||
        current != end)
      {
        return false;
      }

      std::uint32_t tag_id = 0;
      std::string size;
      std::string format;
      bool test_request = false;
      bool profiling_available = false;
      NumberArray full_freq_caps;
      if (!String::StringManip::str_to_int(tag_id_token, tag_id) ||
        !parse_escaped_string_(size_token, size) ||
        !parse_escaped_string_(format_token, format) ||
        !parse_bool_(test_request_token, test_request) ||
        !parse_bool_(profiling_available_token, profiling_available) ||
        !parse_uint32_list_(full_freq_caps_token, full_freq_caps))
      {
        return false;
      }

      value = RequestBasicChannelsInnerData::AdSelectProps(
        tag_id,
        size,
        format,
        test_request,
        profiling_available,
        full_freq_caps);
      return true;
    }

    bool
    parse_trigger_matches_(
      std::string_view token,
      RequestBasicChannelsInnerData::TriggerMatchArray& values)
    {
      constexpr std::size_t EXPECTED_TRIGGER_MATCH_SIZE = 17;

      RequestBasicChannelsInnerData::TriggerMatchArray result;
      result.reserve(token.size() / EXPECTED_TRIGGER_MATCH_SIZE + 1);

      const char* current = token.begin();
      const char* const end = token.end();
      while (current != end)
      {
        const char* const channel_id_end = find_separator_(current, end, TRIGGER_MATCH_SEP);
        if (channel_id_end == end)
        {
          return false;
        }

        const char* const channel_trigger_id_begin = channel_id_end + 1;
        const char* const channel_trigger_id_end =
          find_separator_(channel_trigger_id_begin, end, ',');
        const std::string_view channel_id_token(current, channel_id_end - current);
        const std::string_view channel_trigger_id_token(
          channel_trigger_id_begin,
          channel_trigger_id_end - channel_trigger_id_begin);

        std::uint32_t channel_id = 0;
        std::uint32_t channel_trigger_id = 0;
        if (!String::StringManip::str_to_int(channel_id_token, channel_id) ||
          !String::StringManip::str_to_int(channel_trigger_id_token, channel_trigger_id))
        {
          return false;
        }

        result.emplace_back(channel_id, channel_trigger_id);

        current = channel_trigger_id_end;
        if (current != end && ++current == end)
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
    const auto raw_token = is.read_token();
    const std::string_view token(raw_token.data(), raw_token.size());
    if (is.good() && !parse_ad_bid_slot_impression_(token, value))
    {
      is.setstate(std::ios_base::failbit);
    }
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(
    FixedBufStream<TabCategory>& is,
    RequestBasicChannelsInnerData::AdBidSlotImpressionList& values)
  {
    const auto raw_token = is.read_token();
    const std::string_view token(raw_token.data(), raw_token.size());
    if (is.good())
    {
      if (token == "-")
      {
        values.clear();
        return is;
      }

      constexpr std::size_t EXPECTED_IMPRESSION_SIZE = 32;
      RequestBasicChannelsInnerData::AdBidSlotImpressionList result;
      result.reserve(token.size() / EXPECTED_IMPRESSION_SIZE + 1);

      const char* current = token.begin();
      const char* const end = token.end();
      while (current != end)
      {
        std::string_view impression_token;
        RequestBasicChannelsInnerData::AdBidSlotImpression impression;
        if (!read_component_(current, end, ABSI_SEP2, impression_token) ||
          !parse_ad_bid_slot_impression_(impression_token, impression))
        {
          is.setstate(std::ios_base::failbit);
          return is;
        }
        result.push_back(std::move(impression));
      }

      values.swap(result);
    }

    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(
    FixedBufStream<TabCategory>& is,
    RequestBasicChannelsInnerData::TriggerMatchArray& values)
  {
    const auto raw_token = is.read_token();
    const std::string_view token(raw_token.data(), raw_token.size());
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
  operator>>(FixedBufStream<TabCategory>& is, RequestBasicChannelsInnerData::AdRequestProps& value)
  {
    // inplace loading from stream
    value.data_ = new RequestBasicChannelsInnerData::AdRequestProps::Data;
    is >> value.data_->sizes;
    if (is.fail())
    {
      return is;
    }
    is >> value.data_->country_code;
    if (is.fail())
    {
      return is;
    }
    is >> value.data_->max_text_ads;
    if (is.fail())
    {
      return is;
    }
    is >> value.data_->text_ad_cost_threshold;
    if (is.fail())
    {
      return is;
    }
    is >> value.data_->display_ad_shown;
    if (is.fail())
    {
      return is;
    }
    is >> value.data_->text_ad_shown;
    if (is.fail())
    {
      return is;
    }
    is >> value.data_->ad_select;
    if (is.fail())
    {
      return is;
    }
    value.data_->auction_type = get_auction_type(is);
    value.normalize();
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestBasicChannelsInnerData::AdSelectProps& value)
    /*throw(eh::Exception)*/
  {
    const auto raw_token = is.read_token();
    const std::string_view token(raw_token.data(), raw_token.size());
    if (is.good() && !parse_ad_select_props_(token, value))
    {
      is.setstate(std::ios_base::failbit);
    }

    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(
    FixedBufStream<TabCategory>& is,
    RequestBasicChannelsInnerData::AdSlotImpression& value)
  {
    const auto raw_token = is.pop_token();
    const std::string_view token(raw_token.data(), raw_token.size());
    if (token.empty())
    {
      is.setstate(std::ios_base::eofbit);
    }
    else
    {
      if (!parse_ad_slot_impression_(token, value))
      {
        is.setstate(std::ios_base::failbit);
      }
    }

    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestBasicChannelsInnerData::Match& match_request)
  {
    // inplace loading from stream
    match_request.data_ = new RequestBasicChannelsInnerData::Match::Data;
    const auto raw_history_channels_token = is.read_token();
    const std::string_view history_channels_token(
      raw_history_channels_token.data(),
      raw_history_channels_token.size());
    if (is.good() && !parse_uint32_list_(
      history_channels_token, match_request.data_->history_channels))
    {
      is.setstate(std::ios_base::failbit);
      return is;
    }
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
  operator<<(std::ostream& os, const RequestBasicChannelsInnerData::TriggerMatch& match)
  {
    os << match.channel_id << TRIGGER_MATCH_SEP;
    os << match.channel_trigger_id;
    return os;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestBasicChannelsInnerData::TriggerMatch& match)
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
  operator<<(std::ostream& os, const RequestBasicChannelsInnerData::AdSlotImpression& ad_imp)
  {
    os << ad_imp.data_->revenue << ASI_SEP1;
    os << ad_imp.data_->impression_channels;
    return os;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestBasicChannelsInnerData::AdSlotImpression& ad_imp)
  {
    out << ad_imp.revenue() << ASI_SEP1 << ad_imp.impression_channels();
    return out;
  }

  std::ostream&
  operator<<(std::ostream& os, const RequestBasicChannelsInnerData::AdBidSlotImpression& absi)
  {
    os << absi.data_->revenue << ABSI_SEP1;
    os << absi.data_->revenue_bid << ABSI_SEP1;
    os << absi.data_->impression_channels;
    return os;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestBasicChannelsInnerData::AdBidSlotImpression& absi)
  {
    out << absi.revenue() << ABSI_SEP1
      << absi.revenue_bid() << ABSI_SEP1 << absi.impression_channels();
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
  operator<<(std::ostream& os, const RequestBasicChannelsInnerData::AdRequestProps& ad_req)
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
  operator<<(BufferWriter& out, const RequestBasicChannelsInnerData::AdRequestProps& ad_req)
  {
    out << ad_req.sizes() << '\t'
      << StringIoWrapperOptional(ad_req.country_code()) << '\t'
      << ad_req.max_text_ads() << '\t'
      << ad_req.text_ad_cost_threshold() << '\t'
      << ad_req.display_ad_shown() << '\t'
      << ad_req.text_ad_shown() << '\t'
      << ad_req.ad_select() << '\t' << put_auction_type(ad_req.auction_type());
    return out;
  }

  std::ostream&
  operator<<(std::ostream& os, const RequestBasicChannelsInnerData::AdSelectProps& ad_select)
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
  operator<<(BufferWriter& out, const RequestBasicChannelsInnerData::AdSelectProps& ad_select)
  {
    out << ad_select.tag_id() << AD_SELECT_FIELD_SEPARATOR
      << SpacesMarksString(ad_select.size()) << AD_SELECT_FIELD_SEPARATOR
      << SpacesMarksString(ad_select.format()) << AD_SELECT_FIELD_SEPARATOR
      << ad_select.test_request() << AD_SELECT_FIELD_SEPARATOR
      << ad_select.profiling_available() << AD_SELECT_FIELD_SEPARATOR << ad_select.full_freq_caps();
    return out;
  }

  std::ostream&
  operator<<(std::ostream& os, const RequestBasicChannelsInnerData::Match& match_request)
  {
    os << match_request.data_->history_channels << '\t';
    os << match_request.data_->page_trigger_channels << '\t';
    os << match_request.data_->search_trigger_channels << '\t';
    os << match_request.data_->url_trigger_channels << '\t';
    os << match_request.data_->url_keyword_trigger_channels;
    return os;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestBasicChannelsInnerData::Match& match_request)
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
      << data.holder_->match_request << '\t' << data.holder_->ad_request;
    return out;
  }
} // namespace AdServer::LogProcessing
