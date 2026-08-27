#include "Request_v371.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  const RequestData_V_3_7_1::DeliveryThresholdT
    RequestData_V_3_7_1::DataHolder::max_delivery_threshold_value_("1.00000");

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestData_V_3_7_1& data)
    /*throw(eh::Exception)*/
  {
    data.holder_ = new RequestData_V_3_7_1::DataHolder;
    TokenizerInputArchive<> ia(is);
    ia >> *data.holder_;
    return is;
  }

} // namespace AdServer::LogProcessing
