
#include <algorithm>
#include "RequestBasicChannels.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* RequestBasicChannelsTraits::B::base_name_ =
  "RequestBasicChannels";

template <> const char* RequestBasicChannelsTraits::B::signature_ =
  "RequestBasicChannels";

template <>
const char* RequestBasicChannelsTraits::B::current_version_ = "3.4";

namespace
{
  const char TRIGGER_MATCH_SEP = ':';

  const char PHM_SEP = ':';

  const char ASI_SEP1 = ':';
  const char ASI_SEP2 = '/';

  const char ABSI_SEP1 = ':';
  const char ABSI_SEP2 = '/';

  const char AD_SELECT_FIELD_SEPARATOR = ':';
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
  RequestBasicChannelsInnerData::AdBidSlotImpression& value
)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    FixedBufStream<SemiCategory> stream(token);
    value.data_ =
      new RequestBasicChannelsInnerData::AdBidSlotImpression::Data();
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
  RequestBasicChannelsInnerData::AdBidSlotImpressionList& values
)
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

FixedBufStream<CommaCategory>&
operator>>(
  FixedBufStream<CommaCategory>& is,
  RequestBasicChannelsInnerData::TriggerMatch& match
)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    FixedBufStream<SemiCategory> stream(token);
    stream >> match.channel_id;
    stream >> match.channel_trigger_id;
    is.take_fails(stream);
  }
  return is;
}

FixedBufStream<CommaCategory>&
operator>>(
  FixedBufStream<CommaCategory>& is,
  RequestBasicChannelsInnerData_V_2_7::PartlyHistoryMatch& match
)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    FixedBufStream<SemiCategory> stream(token);
    stream >> match.channel_id;
    stream >> match.visits;
    stream >> match.minimum_visits;
    is.take_fails(stream);
  }
  return is;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  RequestBasicChannelsInnerData_V_2_7::AdRequestProps& value
)
{
  // inplace loading from stream
  value.data_ =
    new RequestBasicChannelsInnerData_V_2_7::AdRequestProps::Data;
  is >> value.data_->size;
  is >> value.data_->country_code;
  is >> value.data_->max_text_ads;
  is >> value.data_->text_ad_cost_threshold;
  is >> value.data_->display_ad_shown;
  is >> value.data_->text_ad_shown;
  value.normalize();
  return is;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  RequestBasicChannelsInnerData_V_3_1::AdRequestProps& value
)
{
  // inplace loading from stream
  value.data_ = new RequestBasicChannelsInnerData_V_3_1::AdRequestProps::Data;
  is >> value.data_->size;
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
  is >> value.data_->ad_select;
  value.normalize();
  return is;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  RequestBasicChannelsInnerData_V_3_3::AdRequestProps& value
)
{
  // inplace loading from stream
  value.data_ = new RequestBasicChannelsInnerData_V_3_3::AdRequestProps::Data;
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
  is >> value.data_->ad_select;
  value.normalize();
  return is;
}

using namespace CampaignSvcs;

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  RequestBasicChannelsInnerData::AdRequestProps& value
)
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
  RequestBasicChannelsInnerData_V_3_1::AdSelectProps& value
)
  /*throw(eh::Exception)*/
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    FixedBufStream<SemiCategory> stream(token);
    value.data_ =
      new RequestBasicChannelsInnerData_V_3_1::AdSelectProps::Data();
    stream >> value.data_->tag_id;
    stream >> value.data_->format;
    stream >> value.data_->test_request;
    stream >> value.data_->profiling_available;
    stream >> value.data_->full_freq_caps;
    is.take_fails(stream);
  }
  return is;
}

// TODO: maybe template them because these 2 are same
FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  RequestBasicChannelsInnerData_V_3_3::AdSelectProps& value
)
  /*throw(eh::Exception)*/
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    FixedBufStream<SemiCategory> stream(token);
    value.data_ =
      new RequestBasicChannelsInnerData_V_3_3::AdSelectProps::Data();
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
  RequestBasicChannelsInnerData::AdSelectProps& value
)
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
  RequestBasicChannelsInnerData::AdSlotImpression& value
)
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

// TODO: maybe template them too
FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  RequestBasicChannelsInnerData_V_3_3::Match& match_request
)
{
  // inplace loading from stream
  match_request.data_ = new RequestBasicChannelsInnerData_V_3_3::Match::Data;
  is >> match_request.data_->history_channels;
  is >> match_request.data_->page_trigger_channels;
  is >> match_request.data_->search_trigger_channels;
  is >> match_request.data_->url_trigger_channels;
  is >> match_request.data_->url_keyword_trigger_channels;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  RequestBasicChannelsInnerData::Match& match_request
)
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
  std::string record;
  record.reserve(1024);
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

std::istream&
operator>>(std::istream& is, RequestBasicChannelsInnerData_V_2_7& data)
  /*throw(eh::Exception)*/
{
  data.holder_ = new RequestBasicChannelsInnerData_V_2_7::DataHolder();
  std::string record;
  record.reserve(1024);
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

std::istream&
operator>>(std::istream& is, RequestBasicChannelsInnerData_V_3_1& data)
  /*throw(eh::Exception)*/
{
  data.holder_ = new RequestBasicChannelsInnerData_V_3_1::DataHolder();
  std::string record;
  record.reserve(1024);
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

// TODO: maybe template them, because all 4 look same
std::istream&
operator>>(std::istream& is, RequestBasicChannelsInnerData_V_3_3& data)
  /*throw(eh::Exception)*/
{
  data.holder_ = new RequestBasicChannelsInnerData_V_3_3::DataHolder();
  std::string record;
  record.reserve(1024);
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
  const RequestBasicChannelsInnerData::TriggerMatch& match
)
{
  os << match.channel_id << TRIGGER_MATCH_SEP;
  os << match.channel_trigger_id;
  return os;
}

std::ostream&
operator<<(
  std::ostream& os,
  const RequestBasicChannelsInnerData::AdSlotImpression& ad_imp
)
{
  os << ad_imp.data_->revenue << ASI_SEP1;
  os << ad_imp.data_->impression_channels;
  return os;
}

std::ostream&
operator<<(
  std::ostream& os,
  const RequestBasicChannelsInnerData::AdBidSlotImpression& absi
)
{
  os << absi.data_->revenue << ABSI_SEP1;
  os << absi.data_->revenue_bid << ABSI_SEP1;
  os << absi.data_->impression_channels;
  return os;
}

std::ostream&
operator<<(
  std::ostream& os,
  const RequestBasicChannelsInnerData::AdBidSlotImpressionList& absi_list
)
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

std::ostream&
operator<<(
  std::ostream& os,
  const RequestBasicChannelsInnerData::AdRequestProps& ad_req
)
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

std::ostream&
operator<<(
  std::ostream& os,
  const RequestBasicChannelsInnerData::AdSelectProps& ad_select
)
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

std::ostream&
operator<<(
  std::ostream& os,
  const RequestBasicChannelsInnerData::Match& match_request
)
{
  os << match_request.data_->history_channels << '\t';
  os << match_request.data_->page_trigger_channels << '\t';
  os << match_request.data_->search_trigger_channels << '\t';
  os << match_request.data_->url_trigger_channels << '\t';
  os << match_request.data_->url_keyword_trigger_channels << '\t';
  return os;
}

std::ostream&
operator<<(std::ostream& os, const RequestBasicChannelsInnerData& data)
  /*throw(eh::Exception)*/
{
  data.holder_->invariant();
  os << data.holder_->user_type << '\t';
  os << data.holder_->user_id << '\t';
  os << data.holder_->temporary_user_id << '\t';
  os << data.holder_->match_request << '\t';
  os << data.holder_->ad_request;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer
