
#include "Request.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* RequestTraits::B::base_name_ = "Request";
  template <> const char* RequestTraits::B::signature_ = "Request";
  template <> const char* RequestTraits::B::current_version_ = "3.7.4";

  namespace {

  const char CC_SEP1 = ',';
  const char CC_SEP2 = ':';

  const char UP_SEP1 = ',';
  const char UP_SEP2 = '=';

  } // namespace

  const RequestData::DeliveryThresholdT
    RequestData::DataHolder::max_delivery_threshold_value_("1.00000");

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestData& data)
    /*throw(eh::Exception)*/
  {
    data.holder_ = new RequestData::DataHolder;
    TokenizerInputArchive</*Aux_::OwnInvariants*/> ia(is);
    data.holder_->serialize_v_3_7_2(ia);
    ia & data.holder_->page_keywords;
    StringArray expected_post_actions;
    ia ^ expected_post_actions;
    data.holder_->expected_post_actions = expected_post_actions.empty() ?
      empty_expected_post_actions() :
      std::make_shared<const StringArray>(std::move(expected_post_actions));
    data.holder_->invariant();
    return is;
  }

  void
  RequestData::read_v_3_7_2_(FixedBufStream<TabCategory>& is)
  {
    holder_ = new DataHolder;
    TokenizerInputArchive<> ia(is);
    holder_->serialize_v_3_7_2(ia);
    holder_->invariant();
  }

  void
  RequestData::read_v_3_7_3_(FixedBufStream<TabCategory>& is)
  {
    holder_ = new DataHolder;
    TokenizerInputArchive<> ia(is);
    holder_->serialize_v_3_7_3(ia);
    holder_->invariant();
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestData_V_3_7_2& data)
    /*throw(eh::Exception)*/
  {
    data.read_(is);
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestData_V_3_7_3& data)
    /*throw(eh::Exception)*/
  {
    data.read_(is);
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestData& data)
    /*throw(eh::Exception)*/
  {
    BufferTabOutputArchive archive(out);
    data.holder_->invariant();
    data.holder_->serialize_v_3_7_2(archive);
    archive & data.holder_->page_keywords;
    archive ^ *data.holder_->expected_post_actions;
    return out;
  }

} // namespace AdServer::LogProcessing
