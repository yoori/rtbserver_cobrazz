
#include "HourlyStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// HourlyStats    
    template<>    
    BasicStats<HourlyStats, 9>::names_type const    
    BasicStats<HourlyStats, 9>::field_names = {    
      ".imps",      
      ".clicks",      
      ".actions",      
      ".requests",      
      ".pub_amount",      
      ".pub_comm_amount",      
      ".adv_amount",      
      ".adv_comm_amount",      
      ".isp_amount"      
    };    
    const char*    
    HourlyStats::tables[2] = {    
      "RequestStatsHourly",      
      "RequestStatsHourlyTest"      
    };    
    void    
    HourlyStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.cc_id_used())      
      {      
        out << sep; 
        out << "cc_id = ";
        if (key_.cc_id_is_null())
          out << "null";
        else
          out << key_.cc_id();        
        sep = fsep;        
      }      
      if (key_.tag_id_used())      
      {      
        out << sep; 
        out << "tag_id = ";
        if (key_.tag_id_is_null())
          out << "null";
        else
          out << key_.tag_id();        
        sep = fsep;        
      }      
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
      if (key_.adv_account_id_used())      
      {      
        out << sep; 
        out << "adv_account_id = ";
        if (key_.adv_account_id_is_null())
          out << "null";
        else
          out << key_.adv_account_id();        
        sep = fsep;        
      }      
      if (key_.pub_account_id_used())      
      {      
        out << sep; 
        out << "pub_account_id = ";
        if (key_.pub_account_id_is_null())
          out << "null";
        else
          out << key_.pub_account_id();        
        sep = fsep;        
      }      
      if (key_.isp_account_id_used())      
      {      
        out << sep; 
        out << "isp_account_id = ";
        if (key_.isp_account_id_is_null())
          out << "null";
        else
          out << key_.isp_account_id();        
        sep = fsep;        
      }      
      if (key_.num_shown_used())      
      {      
        out << sep; 
        out << "num_shown = ";
        if (key_.num_shown_is_null())
          out << "null";
        else
          out << key_.num_shown();        
        sep = fsep;        
      }      
      if (key_.fraud_correction_used())      
      {      
        out << sep; 
        out << "fraud_correction = ";
        if (key_.fraud_correction_is_null())
          out << "null";
        else
          out << (key_.fraud_correction() ? "true" : "false");        
        sep = fsep;        
      }      
      if (key_.stimestamp_used())      
      {      
        out << sep; 
        out << "stimestamp = '";
        if (key_.stimestamp_is_null())
          out << "null";
        else
          out <<  key_.stimestamp().get_gm_time().format("%Y-%m-%d:%H") << '\'';        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    HourlyStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_HourlyStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.cc_id_used())        
        {        
          if (key_.cc_id_is_null())          
          {          
            kstr << sep << "cc_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "cc_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.tag_id_used())        
        {        
          if (key_.tag_id_is_null())          
          {          
            kstr << sep << "tag_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "tag_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
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
        if (key_.adv_account_id_used())        
        {        
          if (key_.adv_account_id_is_null())          
          {          
            kstr << sep << "adv_account_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "adv_account_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.pub_account_id_used())        
        {        
          if (key_.pub_account_id_is_null())          
          {          
            kstr << sep << "pub_account_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "pub_account_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.isp_account_id_used())        
        {        
          if (key_.isp_account_id_is_null())          
          {          
            kstr << sep << "isp_account_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "isp_account_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.num_shown_used())        
        {        
          if (key_.num_shown_is_null())          
          {          
            kstr << sep << "num_shown is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "num_shown = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.fraud_correction_used())        
        {        
          if (key_.fraud_correction_is_null())          
          {          
            kstr << sep << "fraud_correction is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "fraud_correction = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.stimestamp_used())        
        {        
          if (key_.stimestamp_is_null())          
          {          
            kstr << sep << "stimestamp is null";            
            sep = fsep;            
          }          
          else if (key_.stimestamp() == Generics::Time::ZERO)          
          {          
            kstr << sep << "stimestamp = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "stimestamp = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        key_HourlyStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(imps), "          
          "SUM(clicks), "          
          "SUM(actions), "          
          "SUM(requests), "          
          "SUM(pub_amount), "          
          "SUM(pub_comm_amount), "          
          "SUM(adv_amount), "          
          "SUM(adv_comm_amount), "          
          "SUM(isp_amount) "          
        "FROM " << tables[table_] << " "        
        "WHERE ";        
      qstr << key_HourlyStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.cc_id_used() && !key_.cc_id_is_null())      
        query.set(key_.cc_id());        
      if (key_.tag_id_used() && !key_.tag_id_is_null())      
        query.set(key_.tag_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.adv_account_id_used() && !key_.adv_account_id_is_null())      
        query.set(key_.adv_account_id());        
      if (key_.pub_account_id_used() && !key_.pub_account_id_is_null())      
        query.set(key_.pub_account_id());        
      if (key_.isp_account_id_used() && !key_.isp_account_id_is_null())      
        query.set(key_.isp_account_id());        
      if (key_.num_shown_used() && !key_.num_shown_is_null())      
        query.set(key_.num_shown());        
      if (key_.fraud_correction_used() && !key_.fraud_correction_is_null())      
        query.set(key_.fraud_correction());        
      if (key_.stimestamp_used() && !key_.stimestamp_is_null() &&
        key_.stimestamp() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.stimestamp().get_gm_time()));        
      return ask(query);      
    }    
  }  
}
