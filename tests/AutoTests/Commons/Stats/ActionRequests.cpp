
#include "ActionRequests.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ActionRequests    
    template<>    
    BasicStats<ActionRequests, 2>::names_type const    
    BasicStats<ActionRequests, 2>::field_names = {    
      ".count",      
      ".cur_value"      
    };    
    void    
    ActionRequests::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
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
      if (key_.user_status_used())      
      {      
        out << sep; 
        out << "user_status = '";
        if (key_.user_status_is_null())
          out << "null'";
        else
          out << key_.user_status() << '\'';        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    ActionRequests::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ActionRequests;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
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
        if (key_.user_status_used())        
        {        
          if (key_.user_status_is_null())          
          {          
            kstr << sep << "user_status is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "user_status = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_ActionRequests = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(action_request_count), "          
          "SUM(cur_value) "          
        "FROM ActionRequests "        
        "WHERE ";        
      qstr << key_ActionRequests;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.action_id_used() && !key_.action_id_is_null())      
        query.set(key_.action_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.country_code_used() && !key_.country_code_is_null())      
        query.set(key_.country_code());        
      if (key_.action_date_used() && !key_.action_date_is_null() &&
        key_.action_date() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.action_date().get_gm_time()));        
      if (key_.action_referrer_url_used() && !key_.action_referrer_url_is_null())      
        query.set(key_.action_referrer_url());        
      if (key_.user_status_used() && !key_.user_status_is_null())      
        query.set(key_.user_status());        
      return ask(query);      
    }    
  }  
}
