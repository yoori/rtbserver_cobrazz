
#include "Request.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* RequestTraits::B::base_name_ = "Request";
  template <> const char* RequestTraits::B::signature_ = "Request";
  template <> const char* RequestTraits::B::current_version_ = "3.7.2";

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
    ia >> *data.holder_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestData& data)
    /*throw(eh::Exception)*/
  {
    BufferTabOutputArchive archive(out);
    archive << *data.holder_;
    return out;
  }

} // namespace AdServer::LogProcessing
