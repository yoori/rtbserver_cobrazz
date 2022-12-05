
#include "CCGKeywordStatsHourly.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// CCGKeywordStatsHourly    
    template<>    
    BasicStats<CCGKeywordStatsHourly, 5>::names_type const    
    BasicStats<CCGKeywordStatsHourly, 5>::field_names = {    
      ".imps",      
      ".clicks",      
      ".adv_amount",      
      ".adv_comm_amount",      
      ".pub_amount_adv"      
    };    
    void    
    CCGKeywordStatsHourly::print_idname (std::ostream& out) const    
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
      if (key_.ccg_keyword_id_used())      
      {      
        out << sep; 
        out << "ccg_keyword_id = ";
        if (key_.ccg_keyword_id_is_null())
          out << "null";
        else
          out << key_.ccg_keyword_id();        
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
      out << " >" << std::endl;      
    }    
    bool    
    CCGKeywordStatsHourly::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_CCGKeywordStatsHourly;      
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
        if (key_.ccg_keyword_id_used())        
        {        
          if (key_.ccg_keyword_id_is_null())          
          {          
            kstr << sep << "ccg_keyword_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "ccg_keyword_id = :" << ckey++;            
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
        key_CCGKeywordStatsHourly = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(imps), "          
          "SUM(clicks), "          
          "SUM(adv_amount), "          
          "SUM(adv_comm_amount), "          
          "SUM(pub_amount_adv) "          
        "FROM CCGKeywordStatsHourly "        
        "WHERE ";        
      qstr << key_CCGKeywordStatsHourly;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.sdate_used() && !key_.sdate_is_null() &&
        key_.sdate() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.sdate().get_gm_time()));        
      if (key_.adv_sdate_used() && !key_.adv_sdate_is_null() &&
        key_.adv_sdate() != Generics::Time::ZERO)      
        query.set(key_.adv_sdate().get_gm_time().get_date());        
      if (key_.ccg_keyword_id_used() && !key_.ccg_keyword_id_is_null())      
        query.set(key_.ccg_keyword_id());        
      if (key_.currency_exchange_id_used() && !key_.currency_exchange_id_is_null())      
        query.set(key_.currency_exchange_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.cc_id_used() && !key_.cc_id_is_null())      
        query.set(key_.cc_id());        
      return ask(query);      
    }    
  }  
}
