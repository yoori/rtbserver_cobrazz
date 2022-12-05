
#include "CMPRequestStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// CMPRequestStats    
    template<>    
    BasicStats<CMPRequestStats, 5>::names_type const    
    BasicStats<CMPRequestStats, 5>::field_names = {    
      ".imps",      
      ".adv_amount_cmp",      
      ".cmp_amount",      
      ".cmp_amount_global",      
      ".clicks"      
    };    
    void    
    CMPRequestStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.sdate_used())      
      {      
        out << sep; 
        out << "sdate = '";
        if (key_.sdate_is_null())
          out << "null";
        else
          out <<  key_.sdate().get_gm_time().format("%Y-%m-%d:%H") << '\'';        
        sep = fsep;        
      }      
      if (key_.adv_sdate_used())      
      {      
        out << sep; 
        out << "adv_sdate = '";
        if (key_.adv_sdate_is_null())
          out << "null";
        else
          out <<  key_.adv_sdate().get_gm_time().format("%Y-%m-%d") << '\'';        
        sep = fsep;        
      }      
      if (key_.cmp_sdate_used())      
      {      
        out << sep; 
        out << "cmp_sdate = '";
        if (key_.cmp_sdate_is_null())
          out << "null";
        else
          out <<  key_.cmp_sdate().get_gm_time().format("%Y-%m-%d") << '\'';        
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
      if (key_.cmp_account_id_used())      
      {      
        out << sep; 
        out << "cmp_account_id = ";
        if (key_.cmp_account_id_is_null())
          out << "null";
        else
          out << key_.cmp_account_id();        
        sep = fsep;        
      }      
      if (key_.ccg_id_used())      
      {      
        out << sep; 
        out << "ccg_id = ";
        if (key_.ccg_id_is_null())
          out << "null";
        else
          out << key_.ccg_id();        
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
      if (key_.channel_id_used())      
      {      
        out << sep; 
        out << "channel_id = ";
        if (key_.channel_id_is_null())
          out << "null";
        else
          out << key_.channel_id();        
        sep = fsep;        
      }      
      if (key_.channel_rate_id_used())      
      {      
        out << sep; 
        out << "channel_rate_id = ";
        if (key_.channel_rate_id_is_null())
          out << "null";
        else
          out << key_.channel_rate_id();        
        sep = fsep;        
      }      
      if (key_.currency_exchange_id_used())      
      {      
        out << sep; 
        out << "currency_exchange_id = ";
        if (key_.currency_exchange_id_is_null())
          out << "null";
        else
          out << key_.currency_exchange_id();        
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
      if (key_.country_code_used())      
      {      
        out << sep; 
        out << "country_code = '";
        if (key_.country_code_is_null())
          out << "null'";
        else
          out << key_.country_code() << '\'';        
        sep = fsep;        
      }      
      if (key_.walled_garden_used())      
      {      
        out << sep; 
        out << "walled_garden = ";
        if (key_.walled_garden_is_null())
          out << "null";
        else
          out << (key_.walled_garden() ? "true" : "false");        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    CMPRequestStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_CMPRequestStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.sdate_used())        
        {        
          if (key_.sdate_is_null())          
          {          
            kstr << sep << "sdate is null";            
            sep = fsep;            
          }          
          else if (key_.sdate() == Generics::Time::ZERO)          
          {          
            kstr << sep << "sdate = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "sdate = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.adv_sdate_used())        
        {        
          if (key_.adv_sdate_is_null())          
          {          
            kstr << sep << "adv_sdate is null";            
            sep = fsep;            
          }          
          else if (key_.adv_sdate() == Generics::Time::ZERO)          
          {          
            kstr << sep << "adv_sdate = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "adv_sdate = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.cmp_sdate_used())        
        {        
          if (key_.cmp_sdate_is_null())          
          {          
            kstr << sep << "cmp_sdate is null";            
            sep = fsep;            
          }          
          else if (key_.cmp_sdate() == Generics::Time::ZERO)          
          {          
            kstr << sep << "cmp_sdate = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "cmp_sdate = :"<< ckey++;            
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
        if (key_.cmp_account_id_used())        
        {        
          if (key_.cmp_account_id_is_null())          
          {          
            kstr << sep << "cmp_account_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "cmp_account_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.ccg_id_used())        
        {        
          if (key_.ccg_id_is_null())          
          {          
            kstr << sep << "ccg_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "ccg_id = :" << ckey++;            
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
        if (key_.channel_id_used())        
        {        
          if (key_.channel_id_is_null())          
          {          
            kstr << sep << "channel_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "channel_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.channel_rate_id_used())        
        {        
          if (key_.channel_rate_id_is_null())          
          {          
            kstr << sep << "channel_rate_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "channel_rate_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.currency_exchange_id_used())        
        {        
          if (key_.currency_exchange_id_is_null())          
          {          
            kstr << sep << "currency_exchange_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "currency_exchange_id = :" << ckey++;            
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
        if (key_.country_code_used())        
        {        
          if (key_.country_code_is_null())          
          {          
            kstr << sep << "country_code is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "country_code = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.walled_garden_used())        
        {        
          if (key_.walled_garden_is_null())          
          {          
            kstr << sep << "walled_garden is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "walled_garden = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_CMPRequestStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(imps), "          
          "SUM(adv_amount_cmp), "          
          "SUM(cmp_amount), "          
          "SUM(cmp_amount_global), "          
          "SUM(clicks) "          
        "FROM CMPRequestStatsHourly "        
        "WHERE ";        
      qstr << key_CMPRequestStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.sdate_used() && !key_.sdate_is_null() &&
        key_.sdate() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.sdate().get_gm_time()));        
      if (key_.adv_sdate_used() && !key_.adv_sdate_is_null() &&
        key_.adv_sdate() != Generics::Time::ZERO)      
        query.set(key_.adv_sdate().get_gm_time().get_date());        
      if (key_.cmp_sdate_used() && !key_.cmp_sdate_is_null() &&
        key_.cmp_sdate() != Generics::Time::ZERO)      
        query.set(key_.cmp_sdate().get_gm_time().get_date());        
      if (key_.adv_account_id_used() && !key_.adv_account_id_is_null())      
        query.set(key_.adv_account_id());        
      if (key_.cmp_account_id_used() && !key_.cmp_account_id_is_null())      
        query.set(key_.cmp_account_id());        
      if (key_.ccg_id_used() && !key_.ccg_id_is_null())      
        query.set(key_.ccg_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.channel_id_used() && !key_.channel_id_is_null())      
        query.set(key_.channel_id());        
      if (key_.channel_rate_id_used() && !key_.channel_rate_id_is_null())      
        query.set(key_.channel_rate_id());        
      if (key_.currency_exchange_id_used() && !key_.currency_exchange_id_is_null())      
        query.set(key_.currency_exchange_id());        
      if (key_.fraud_correction_used() && !key_.fraud_correction_is_null())      
        query.set(key_.fraud_correction());        
      if (key_.cc_id_used() && !key_.cc_id_is_null())      
        query.set(key_.cc_id());        
      if (key_.country_code_used() && !key_.country_code_is_null())      
        query.set(key_.country_code());        
      if (key_.walled_garden_used() && !key_.walled_garden_is_null())      
        query.set(key_.walled_garden());        
      return ask(query);      
    }    
  }  
}
