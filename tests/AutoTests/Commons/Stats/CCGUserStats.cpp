
#include "CCGUserStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// CCGUserStats    
    template<>    
    BasicStats<CCGUserStats, 2>::names_type const    
    BasicStats<CCGUserStats, 2>::field_names = {    
      ".unique_users",      
      ".control_sum"      
    };    
    void    
    CCGUserStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
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
      if (key_.last_appearance_date_used())      
      {      
        out << sep; 
        out << "last_appearance_date = '";
        if (key_.last_appearance_date_is_null())
          out << "null";
        else
          out <<  key_.last_appearance_date().get_gm_time().format("%Y-%m-%d") << '\'';        
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
      out << " >" << std::endl;      
    }    
    bool    
    CCGUserStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_CCGUserStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
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
        if (key_.last_appearance_date_used())        
        {        
          if (key_.last_appearance_date_is_null())          
          {          
            kstr << sep << "last_appearance_date is null";            
            sep = fsep;            
          }          
          else if (key_.last_appearance_date() == Generics::Time::ZERO)          
          {          
            kstr << sep << "last_appearance_date = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "last_appearance_date = :"<< ckey++;            
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
        key_CCGUserStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(unique_users), "          
          "SUM( ((adv_sdate - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(adv_sdate - nullif(last_appearance_date, '-infinity'), 32)) * unique_users) "          
        "FROM CCGUserStats "        
        "WHERE ";        
      qstr << key_CCGUserStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.ccg_id_used() && !key_.ccg_id_is_null())      
        query.set(key_.ccg_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.last_appearance_date_used() && !key_.last_appearance_date_is_null() &&
        key_.last_appearance_date() != Generics::Time::ZERO)      
        query.set(key_.last_appearance_date().get_gm_time().get_date());        
      if (key_.adv_sdate_used() && !key_.adv_sdate_is_null() &&
        key_.adv_sdate() != Generics::Time::ZERO)      
        query.set(key_.adv_sdate().get_gm_time().get_date());        
      return ask(query);      
    }    
  }  
}
