
#include "Request_v360.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

const RequestData_V_3_6::DeliveryThresholdT
  RequestData_V_3_6::DataHolder::max_delivery_threshold_value_("1.00000");

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, RequestData_V_3_6& data)
  /*throw(eh::Exception)*/
{
  data.holder_ = new RequestData_V_3_6::DataHolder;
  TokenizerInputArchive<> ia(is);
  ia >> *data.holder_;
  return is;
}

} // namespace LogProcessing
} // namespace AdServer
