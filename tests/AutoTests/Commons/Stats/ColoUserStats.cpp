
#include "ColoUserStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ColoUserStats    
    template<>    
    BasicStats<ColoUserStats, 12>::names_type const    
    BasicStats<ColoUserStats, 12>::field_names = {    
      ".unique_users",      
      ".network_unique_users",      
      ".profiling_unique_users",      
      ".unique_hids",      
      ".control_unique_users",      
      ".control_network_unique_users",      
      ".control_profiling_unique_users",      
      ".control_unique_hids",      
      ".neg_control_unique_users",      
      ".neg_control_network_unique_users",      
      ".neg_control_profiling_unique_users",      
      ".neg_control_unique_hids"      
    };    
    void    
    ColoUserStats::print_idname (std::ostream& out) const    
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
      if (key_.isp_sdate_used())      
      {      
        out << sep; 
        out << "isp_sdate = '";
        if (key_.isp_sdate_is_null())
          out << "null";
        else
          out <<  key_.isp_sdate().get_gm_time().format("%Y-%m-%d") << '\'';        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    ColoUserStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ColoUserStats;      
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
        if (key_.isp_sdate_used())        
        {        
          if (key_.isp_sdate_is_null())          
          {          
            kstr << sep << "isp_sdate is null";            
            sep = fsep;            
          }          
          else if (key_.isp_sdate() == Generics::Time::ZERO)          
          {          
            kstr << sep << "isp_sdate = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "isp_sdate = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        key_ColoUserStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(unique_users), "          
          "SUM(network_unique_users), "          
          "SUM(profiling_unique_users), "          
          "SUM(unique_hids), "          
          "SUM( ((isp_sdate - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(isp_sdate - nullif(last_appearance_date, '-infinity'), 32)) * unique_users), "          
          "SUM( ((isp_sdate - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(isp_sdate - nullif(last_appearance_date, '-infinity'), 32)) * network_unique_users), "          
          "SUM( ((isp_sdate - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(isp_sdate - nullif(last_appearance_date, '-infinity'), 32)) * profiling_unique_users), "          
          "SUM( ((isp_sdate - to_date('01-01-1970', 'DD.MM.YYYY')) * (32 + 1) +  least(isp_sdate - nullif(last_appearance_date, '-infinity'), 32)) * unique_hids), "          
          "SUM(CASE WHEN unique_users < 0 THEN unique_users ELSE 0 END), "          
          "SUM(CASE WHEN network_unique_users < 0 THEN network_unique_users ELSE 0 END), "          
          "SUM(CASE WHEN profiling_unique_users < 0 THEN profiling_unique_users ELSE 0 END), "          
          "SUM(CASE WHEN unique_hids < 0 THEN unique_hids ELSE 0 END) "          
        "FROM ColoUserStats "        
        "WHERE ";        
      qstr << key_ColoUserStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.last_appearance_date_used() && !key_.last_appearance_date_is_null() &&
        key_.last_appearance_date() != Generics::Time::ZERO)      
        query.set(key_.last_appearance_date().get_gm_time().get_date());        
      if (key_.isp_sdate_used() && !key_.isp_sdate_is_null() &&
        key_.isp_sdate() != Generics::Time::ZERO)      
        query.set(key_.isp_sdate().get_gm_time().get_date());        
      return ask(query);      
    }    
  }  
}
