
#include "Request_v361.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

const RequestData_V_3_6_1::DeliveryThresholdT
  RequestData_V_3_6_1::DataHolder::max_delivery_threshold_value_("1.00000");

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, RequestData_V_3_6_1& data)
  /*throw(eh::Exception)*/
{
  data.holder_ = new RequestData_V_3_6_1::DataHolder;
  TokenizerInputArchive<> ia(is);
  ia >> *data.holder_;
  return is;
}

} // namespace LogProcessing
} // namespace AdServer
