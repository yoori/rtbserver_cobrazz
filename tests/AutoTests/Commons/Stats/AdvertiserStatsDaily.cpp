
#include "AdvertiserStatsDaily.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// AdvertiserStatsDaily    
    template<>    
    BasicStats<AdvertiserStatsDaily, 9>::names_type const    
    BasicStats<AdvertiserStatsDaily, 9>::field_names = {    
      ".requests",      
      ".imps",      
      ".clicks",      
      ".actions",      
      ".adv_amount",      
      ".adv_comm_amount",      
      ".pub_amount_adv",      
      ".pub_comm_amount_adv",      
      ".adv_amount_cmp"      
    };    
    void    
    AdvertiserStatsDaily::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
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
      if (key_.creative_id_used())      
      {      
        out << sep; 
        out << "creative_id = ";
        if (key_.creative_id_is_null())
          out << "null";
        else
          out << key_.creative_id();        
        sep = fsep;        
      }      
      if (key_.campaign_id_used())      
      {      
        out << sep; 
        out << "campaign_id = ";
        if (key_.campaign_id_is_null())
          out << "null";
        else
          out << key_.campaign_id();        
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
      if (key_.agency_account_id_used())      
      {      
        out << sep; 
        out << "agency_account_id = ";
        if (key_.agency_account_id_is_null())
          out << "null";
        else
          out << key_.agency_account_id();        
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
    AdvertiserStatsDaily::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_AdvertiserStatsDaily;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
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
        if (key_.creative_id_used())        
        {        
          if (key_.creative_id_is_null())          
          {          
            kstr << sep << "creative_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "creative_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.campaign_id_used())        
        {        
          if (key_.campaign_id_is_null())          
          {          
            kstr << sep << "campaign_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "campaign_id = :" << ckey++;            
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
        if (key_.agency_account_id_used())        
        {        
          if (key_.agency_account_id_is_null())          
          {          
            kstr << sep << "agency_account_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "agency_account_id = :" << ckey++;            
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
        key_AdvertiserStatsDaily = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(requests), "          
          "SUM(imps), "          
          "SUM(clicks), "          
          "SUM(actions), "          
          "SUM(adv_amount), "          
          "SUM(adv_comm_amount), "          
          "SUM(pub_amount_adv), "          
          "SUM(pub_comm_amount_adv), "          
          "SUM(adv_amount_cmp) "          
        "FROM AdvertiserStatsDaily "        
        "WHERE ";        
      qstr << key_AdvertiserStatsDaily;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.adv_sdate_used() && !key_.adv_sdate_is_null() &&
        key_.adv_sdate() != Generics::Time::ZERO)      
        query.set(key_.adv_sdate().get_gm_time().get_date());        
      if (key_.cc_id_used() && !key_.cc_id_is_null())      
        query.set(key_.cc_id());        
      if (key_.creative_id_used() && !key_.creative_id_is_null())      
        query.set(key_.creative_id());        
      if (key_.campaign_id_used() && !key_.campaign_id_is_null())      
        query.set(key_.campaign_id());        
      if (key_.ccg_id_used() && !key_.ccg_id_is_null())      
        query.set(key_.ccg_id());        
      if (key_.adv_account_id_used() && !key_.adv_account_id_is_null())      
        query.set(key_.adv_account_id());        
      if (key_.agency_account_id_used() && !key_.agency_account_id_is_null())      
        query.set(key_.agency_account_id());        
      if (key_.country_code_used() && !key_.country_code_is_null())      
        query.set(key_.country_code());        
      if (key_.walled_garden_used() && !key_.walled_garden_is_null())      
        query.set(key_.walled_garden());        
      return ask(query);      
    }    
  }  
}
