#include "Generics/CompositeMetricsProvider.hpp"
#include <map>

inline void BidStatisticsPrometheusInc(Generics::CompositeMetricsProvider *cmp, int ccg_id)
{
   std::map<std::string,std::string> m;
   m["ccg_id"]=std::to_string(ccg_id);
   cmp->add_value_prometheus("bids",m,1);
}