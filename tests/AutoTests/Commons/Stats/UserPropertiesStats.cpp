
#include "UserPropertiesStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// UserPropertiesStats    
    template<>    
    BasicStats<UserPropertiesStats, 6>::names_type const    
    BasicStats<UserPropertiesStats, 6>::field_names = {    
      ".imps",      
      ".clicks",      
      ".actions",      
      ".requests",      
      ".imps_unverified",      
      ".profiling_requests"      
    };    
    void    
    UserPropertiesStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.name_used())      
      {      
        out << sep; 
        out << "name = '";
        if (key_.name_is_null())
          out << "null'";
        else
          out << key_.name() << '\'';        
        sep = fsep;        
      }      
      if (key_.value_used())      
      {      
        out << sep; 
        out << "value = '";
        if (key_.value_is_null())
          out << "null'";
        else
          out << key_.value() << '\'';        
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
      if (key_.hour_used())      
      {      
        out << sep; 
        out << "hour = ";
        if (key_.hour_is_null())
          out << "null";
        else
          out << key_.hour();        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    UserPropertiesStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_UserPropertiesStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.name_used())        
        {        
          if (key_.name_is_null())          
          {          
            kstr << sep << "name is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "name = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.value_used())        
        {        
          if (key_.value_is_null())          
          {          
            kstr << sep << "value is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "value = :" << ckey++;            
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
        if (key_.hour_used())        
        {        
          if (key_.hour_is_null())          
          {          
            kstr << sep << "hour is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "hour = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_UserPropertiesStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(imps), "          
          "SUM(clicks), "          
          "SUM(actions), "          
          "SUM(requests), "          
          "SUM(imps_unverified), "          
          "SUM(profiling_requests) "          
        "FROM UserPropertyStatsHourly stat "        
        "Left join userproperty up on (up.user_property_id = stat.user_property_id) "        
        "WHERE ";        
      qstr << key_UserPropertiesStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.name_used() && !key_.name_is_null())      
        query.set(key_.name());        
      if (key_.value_used() && !key_.value_is_null())      
        query.set(key_.value());        
      if (key_.user_status_used() && !key_.user_status_is_null())      
        query.set(key_.user_status());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.stimestamp_used() && !key_.stimestamp_is_null() &&
        key_.stimestamp() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.stimestamp().get_gm_time()));        
      if (key_.sdate_used() && !key_.sdate_is_null() &&
        key_.sdate() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.sdate().get_gm_time()));        
      if (key_.hour_used() && !key_.hour_is_null())      
        query.set(key_.hour());        
      return ask(query);      
    }    
  }  
}
