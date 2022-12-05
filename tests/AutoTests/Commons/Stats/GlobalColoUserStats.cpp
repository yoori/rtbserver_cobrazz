
#include "GlobalColoUserStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// GlobalColoUserStats    
    template<>    
    BasicStats<GlobalColoUserStats, 16>::names_type const    
    BasicStats<GlobalColoUserStats, 16>::field_names = {    
      ".unique_users",      
      ".network_unique_users",      
      ".profiling_unique_users",      
      ".unique_hids",      
      ".control_unique_users_1",      
      ".control_unique_users_2",      
      ".control_network_unique_users_1",      
      ".control_network_unique_users_2",      
      ".control_profiling_unique_users_1",      
      ".control_profiling_unique_users_2",      
      ".control_unique_hids_1",      
      ".control_unique_hids_2",      
      ".neg_control_unique_users",      
      ".neg_control_network_unique_users",      
      ".neg_control_profiling_unique_users",      
      ".neg_control_unique_hids"      
    };    
    void    
    GlobalColoUserStats::print_idname (std::ostream& out) const    
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
      if (key_.create_date_used())      
      {      
        out << sep; 
        out << "create_date = '";
        if (key_.create_date_is_null())
          out << "null";
        else
          out <<  key_.create_date().get_gm_time().format("%Y-%m-%d") << '\'';        
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
      if (key_.global_sdate_used())      
      {      
        out << sep; 
        out << "global_sdate = '";
        if (key_.global_sdate_is_null())
          out << "null";
        else
          out <<  key_.global_sdate().get_gm_time().format("%Y-%m-%d") << '\'';        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    GlobalColoUserStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_GlobalColoUserStats;      
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
        if (key_.create_date_used())        
        {        
          if (key_.create_date_is_null())          
          {          
            kstr << sep << "create_date is null";            
            sep = fsep;            
          }          
          else if (key_.create_date() == Generics::Time::ZERO)          
          {          
            kstr << sep << "create_date = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "create_date = :"<< ckey++;            
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
        if (key_.global_sdate_used())        
        {        
          if (key_.global_sdate_is_null())          
          {          
            kstr << sep << "global_sdate is null";            
            sep = fsep;            
          }          
          else if (key_.global_sdate() == Generics::Time::ZERO)          
          {          
            kstr << sep << "global_sdate = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "global_sdate = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        key_GlobalColoUserStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(unique_users), "          
          "SUM(network_unique_users), "          
          "SUM(profiling_unique_users), "          
          "SUM(unique_hids), "          
          "SUM( ((global_sdate - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(global_sdate - nullif(last_appearance_date, '-infinity'), 32)) * unique_users), "          
          "SUM( ((create_date - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(create_date - nullif(last_appearance_date, '-infinity'), 32)) * unique_users), "          
          "SUM( ((global_sdate - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(global_sdate - nullif(last_appearance_date, '-infinity'), 32)) * network_unique_users), "          
          "SUM( ((create_date - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(create_date - nullif(last_appearance_date, '-infinity'), 32)) * network_unique_users), "          
          "SUM( ((global_sdate - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(global_sdate - nullif(last_appearance_date, '-infinity'), 32)) * profiling_unique_users), "          
          "SUM( ((create_date - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(create_date - nullif(last_appearance_date, '-infinity'), 32)) * profiling_unique_users), "          
          "SUM( ((global_sdate - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(global_sdate - nullif(last_appearance_date, '-infinity'), 32)) * unique_hids), "          
          "SUM( ((create_date - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(create_date - nullif(last_appearance_date, '-infinity'), 32)) * unique_hids), "          
          "SUM(CASE WHEN unique_users < 0 THEN unique_users ELSE 0 END), "          
          "SUM(CASE WHEN network_unique_users < 0 THEN network_unique_users ELSE 0 END), "          
          "SUM(CASE WHEN profiling_unique_users < 0 THEN profiling_unique_users ELSE 0 END), "          
          "SUM(CASE WHEN unique_hids < 0 THEN unique_hids ELSE 0 END) "          
        "FROM GlobalColoUserStats "        
        "WHERE ";        
      qstr << key_GlobalColoUserStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.create_date_used() && !key_.create_date_is_null() &&
        key_.create_date() != Generics::Time::ZERO)      
        query.set(key_.create_date().get_gm_time().get_date());        
      if (key_.last_appearance_date_used() && !key_.last_appearance_date_is_null() &&
        key_.last_appearance_date() != Generics::Time::ZERO)      
        query.set(key_.last_appearance_date().get_gm_time().get_date());        
      if (key_.global_sdate_used() && !key_.global_sdate_is_null() &&
        key_.global_sdate() != Generics::Time::ZERO)      
        query.set(key_.global_sdate().get_gm_time().get_date());        
      return ask(query);      
    }    
  }  
}
