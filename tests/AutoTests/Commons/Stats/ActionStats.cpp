
#include "ActionStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ActionStats    
    template<>    
    BasicStats<ActionStats, 1>::names_type const    
    BasicStats<ActionStats, 1>::field_names = {    
      ".count"      
    };    
    void    
    ActionStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.action_request_id_used())      
      {      
        out << sep; 
        out << "action_request_id = ";
        if (key_.action_request_id_is_null())
          out << "null";
        else
          out << key_.action_request_id();        
        sep = fsep;        
      }      
      if (key_.action_id_used())      
      {      
        out << sep; 
        out << "action_id = ";
        if (key_.action_id_is_null())
          out << "null";
        else
          out << key_.action_id();        
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
      if (key_.action_referrer_url_used())      
      {      
        out << sep; 
        out << "action_referrer_url = '";
        if (key_.action_referrer_url_is_null())
          out << "null'";
        else
          out << key_.action_referrer_url() << '\'';        
        sep = fsep;        
      }      
      if (key_.action_date_used())      
      {      
        out << sep; 
        out << "action_date = '";
        if (key_.action_date_is_null())
          out << "null";
        else
          out <<  key_.action_date().get_gm_time().format("%Y-%m-%d:%H") << '\'';        
        sep = fsep;        
      }      
      if (key_.imp_date_used())      
      {      
        out << sep; 
        out << "imp_date = '";
        if (key_.imp_date_is_null())
          out << "null";
        else
          out <<  key_.imp_date().get_gm_time().format("%Y-%m-%d:%H") << '\'';        
        sep = fsep;        
      }      
      if (key_.click_date_used())      
      {      
        out << sep; 
        out << "click_date = '";
        if (key_.click_date_is_null())
          out << "null";
        else
          out <<  key_.click_date().get_gm_time().format("%Y-%m-%d:%H") << '\'';        
        sep = fsep;        
      }      
      if (key_.cur_value_used())      
      {      
        out << sep; 
        out << "cur_value = ";
        if (key_.cur_value_is_null())
          out << "null";
        else
          out << key_.cur_value();        
        sep = fsep;        
      }      
      if (key_.order_id_used())      
      {      
        out << sep; 
        out << "order_id = '";
        if (key_.order_id_is_null())
          out << "null'";
        else
          out << key_.order_id() << '\'';        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    ActionStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ActionStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.action_request_id_used())        
        {        
          if (key_.action_request_id_is_null())          
          {          
            kstr << sep << "action_request_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "action_request_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.action_id_used())        
        {        
          if (key_.action_id_is_null())          
          {          
            kstr << sep << "action_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "action_id = :" << ckey++;            
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
        if (key_.action_referrer_url_used())        
        {        
          if (key_.action_referrer_url_is_null())          
          {          
            kstr << sep << "action_referrer_url is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "action_referrer_url = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.action_date_used())        
        {        
          if (key_.action_date_is_null())          
          {          
            kstr << sep << "action_date is null";            
            sep = fsep;            
          }          
          else if (key_.action_date() == Generics::Time::ZERO)          
          {          
            kstr << sep << "action_date = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "action_date = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.imp_date_used())        
        {        
          if (key_.imp_date_is_null())          
          {          
            kstr << sep << "imp_date is null";            
            sep = fsep;            
          }          
          else if (key_.imp_date() == Generics::Time::ZERO)          
          {          
            kstr << sep << "imp_date = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "imp_date = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.click_date_used())        
        {        
          if (key_.click_date_is_null())          
          {          
            kstr << sep << "click_date is null";            
            sep = fsep;            
          }          
          else if (key_.click_date() == Generics::Time::ZERO)          
          {          
            kstr << sep << "click_date = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "click_date = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.cur_value_used())        
        {        
          if (key_.cur_value_is_null())          
          {          
            kstr << sep << "cur_value is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "TRUNC(cur_value, 2) = TRUNC(:" << ckey++ << ", 2)";            
            sep = fsep;            
          }          
        }        
        if (key_.order_id_used())        
        {        
          if (key_.order_id_is_null())          
          {          
            kstr << sep << "order_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "order_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_ActionStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "COUNT(*) "          
        "FROM ActionStats "        
        "WHERE ";        
      qstr << key_ActionStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.action_request_id_used() && !key_.action_request_id_is_null())      
        query.set(key_.action_request_id());        
      if (key_.action_id_used() && !key_.action_id_is_null())      
        query.set(key_.action_id());        
      if (key_.cc_id_used() && !key_.cc_id_is_null())      
        query.set(key_.cc_id());        
      if (key_.tag_id_used() && !key_.tag_id_is_null())      
        query.set(key_.tag_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.country_code_used() && !key_.country_code_is_null())      
        query.set(key_.country_code());        
      if (key_.action_referrer_url_used() && !key_.action_referrer_url_is_null())      
        query.set(key_.action_referrer_url());        
      if (key_.action_date_used() && !key_.action_date_is_null() &&
        key_.action_date() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.action_date().get_gm_time()));        
      if (key_.imp_date_used() && !key_.imp_date_is_null() &&
        key_.imp_date() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.imp_date().get_gm_time()));        
      if (key_.click_date_used() && !key_.click_date_is_null() &&
        key_.click_date() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.click_date().get_gm_time()));        
      if (key_.cur_value_used() && !key_.cur_value_is_null())      
        query.set(key_.cur_value());        
      if (key_.order_id_used() && !key_.order_id_is_null())      
        query.set(key_.order_id());        
      return ask(query);      
    }    
  }  
}
