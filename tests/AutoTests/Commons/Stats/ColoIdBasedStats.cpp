
#include "ColoIdBasedStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ColoIdBasedStats    
    template<>    
    BasicStats<ColoIdBasedStats, 54>::names_type const    
    BasicStats<ColoIdBasedStats, 54>::field_names = {    
      ".ActionRequests.action_request_count",      
      ".ActionStats.count",      
      ".CCGKeywordStatsHourly.imps",      
      ".CCGKeywordStatsHourly.clicks",      
      ".CCGKeywordStatsHourly.adv_amount",      
      ".CCGKeywordStatsHourly.adv_comm_amount",      
      ".CCGKeywordStatsHourly.pub_amount_adv",      
      ".CCGUserStats.unique_users",      
      ".CMPRequestStatsHourly.imps",      
      ".CMPRequestStatsHourly.adv_amount_cmp",      
      ".CMPRequestStatsHourly.cmp_amount",      
      ".CMPRequestStatsHourly.cmp_amount_global",      
      ".CMPRequestStatsHourly.clicks",      
      ".CampaignUserStats.unique_users",      
      ".CCUserStats.unique_users",      
      ".ChannelTriggerStats.hits",      
      ".ChannelUsageStats.imps",      
      ".ChannelUsageStats.clicks",      
      ".ChannelUsageStats.actions",      
      ".ChannelUsageStats.revenue",      
      ".ColoUserStats.unique_users",      
      ".ColoUserStats.network_unique_users",      
      ".ExpressionPerformance.imps_verified",      
      ".ExpressionPerformance.clicks",      
      ".ExpressionPerformance.actions",      
      ".GlobalColoUserStats.unique_users",      
      ".GlobalColoUserStats.network_unique_users",      
      ".PassbackStats.requests",      
      ".RequestStatsHourly.imps",      
      ".RequestStatsHourly.requests",      
      ".RequestStatsHourly.clicks",      
      ".RequestStatsHourly.actions",      
      ".RequestStatsHourly.adv_amount",      
      ".RequestStatsHourly.adv_amount_global",      
      ".RequestStatsHourly.pub_amount",      
      ".RequestStatsHourly.pub_amount_global",      
      ".RequestStatsHourly.isp_amount",      
      ".RequestStatsHourly.isp_amount_global",      
      ".RequestStatsHourly.adv_comm_amount",      
      ".RequestStatsHourly.pub_comm_amount",      
      ".RequestStatsHourly.adv_comm_amount_global",      
      ".RequestStatsHourly.pub_comm_amount_global",      
      ".RequestStatsHourly.adv_invoice_comm_amount",      
      ".RequestStatsHourly.passbacks",      
      ".SiteChannelStats.imps",      
      ".SiteChannelStats.adv_revenue",      
      ".SiteChannelStats.pub_revenue",      
      ".UserPropertyStatsHourly.requests",      
      ".UserPropertyStatsHourly.imps",      
      ".UserPropertyStatsHourly.clicks",      
      ".UserPropertyStatsHourly.actions",      
      ".UserPropertyStatsHourly.imps_unverified",      
      ".UserPropertyStatsHourly.profiling_requests",      
      ".TagAuctionStats.requests"      
    };    
    void    
    ColoIdBasedStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.colo_id_used())      
      {      
        out << sep; 
        out << "colo_id = ";
        if (key_.colo_id_is_null())
          out << "null";
        else
          out << key_.colo_id();        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    ColoIdBasedStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ColoIdBasedStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.colo_id_used())        
        {        
          if (key_.colo_id_is_null())          
          {          
            kstr << sep << "colo_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "colo_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_ColoIdBasedStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT * FROM ";        
          qstr << "("          
          "SELECT "          
            "SUM(action_request_count) "            
          "FROM ActionRequests "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS ActionRequests,";          
          qstr << "("          
          "SELECT "          
            "COUNT(action_request_id) "            
          "FROM ActionStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS ActionStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(imps), "            
            "SUM(clicks), "            
            "SUM(adv_amount), "            
            "SUM(adv_comm_amount), "            
            "SUM(pub_amount_adv) "            
          "FROM CCGKeywordStatsHourly "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS CCGKeywordStatsHourly,";          
          qstr << "("          
          "SELECT "          
            "SUM(unique_users) "            
          "FROM CCGUserStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS CCGUserStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(imps), "            
            "SUM(adv_amount_cmp), "            
            "SUM(cmp_amount), "            
            "SUM(cmp_amount_global), "            
            "SUM(clicks) "            
          "FROM CMPRequestStatsHourly "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS CMPRequestStatsHourly,";          
          qstr << "("          
          "SELECT "          
            "SUM(unique_users) "            
          "FROM CampaignUserStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS CampaignUserStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(unique_users) "            
          "FROM CCUserStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS CCUserStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(hits) "            
          "FROM ChannelTriggerStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS ChannelTriggerStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(imps), "            
            "SUM(clicks), "            
            "SUM(actions), "            
            "SUM(revenue) "            
          "FROM ChannelUsageStatsHourly "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS ChannelUsageStatsHourly,";          
          qstr << "("          
          "SELECT "          
            "SUM(unique_users), "            
            "SUM(network_unique_users) "            
          "FROM ColoUserStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS ColoUserStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(imps_verified), "            
            "SUM(clicks), "            
            "SUM(actions) "            
          "FROM ExpressionPerformance "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS ExpressionPerformance,";          
          qstr << "("          
          "SELECT "          
            "SUM(unique_users), "            
            "SUM(network_unique_users) "            
          "FROM GlobalColoUserStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS GlobalColoUserStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(requests) "            
          "FROM PassbackStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS PassbackStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(imps), "            
            "SUM(requests), "            
            "SUM(clicks), "            
            "SUM(actions), "            
            "SUM(adv_amount), "            
            "SUM(adv_amount_global), "            
            "SUM(pub_amount), "            
            "SUM(pub_amount_global), "            
            "SUM(isp_amount), "            
            "SUM(isp_amount_global), "            
            "SUM(adv_comm_amount), "            
            "SUM(pub_comm_amount), "            
            "SUM(adv_comm_amount_global), "            
            "SUM(pub_comm_amount_global), "            
            "SUM(adv_invoice_comm_amount), "            
            "SUM(passbacks) "            
          "FROM RequestStatsHourly "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS RequestStatsHourly,";          
          qstr << "("          
          "SELECT "          
            "SUM(imps), "            
            "SUM(adv_revenue), "            
            "SUM(pub_revenue) "            
          "FROM SiteChannelStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS SiteChannelStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(requests), "            
            "SUM(imps), "            
            "SUM(clicks), "            
            "SUM(actions), "            
            "SUM(imps_unverified), "            
            "SUM(profiling_requests) "            
          "FROM UserPropertyStatsHourly "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS UserPropertyStatsHourly,";          
          qstr << "("          
          "SELECT "          
            "SUM(requests) "            
          "FROM TagAuctionStats "          
          "WHERE ";          
          qstr << key_ColoIdBasedStats << ") AS TagAuctionStats";          
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      return ask(query);      
    }    
  }  
}
