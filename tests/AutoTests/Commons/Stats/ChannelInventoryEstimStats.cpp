
#include "ChannelInventoryEstimStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelInventoryEstimStats    
    template<>    
    BasicStats<ChannelInventoryEstimStats, 2>::names_type const    
    BasicStats<ChannelInventoryEstimStats, 2>::field_names = {    
      ".users_regular",      
      ".users_from_now"      
    };    
    void    
    ChannelInventoryEstimStats::print_idname (std::ostream& out) const    
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
          out <<  key_.sdate().get_gm_time().format("%Y-%m-%d") << '\'';        
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
      if (key_.match_level_used())      
      {      
        out << sep; 
        out << "match_level = ";
        if (key_.match_level_is_null())
          out << "null";
        else
          out << key_.match_level();        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    ChannelInventoryEstimStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ChannelInventoryEstimStats;      
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
        if (key_.match_level_used())        
        {        
          if (key_.match_level_is_null())          
          {          
            kstr << sep << "match_level is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "TRUNC(match_level, 2) = TRUNC(:" << ckey++ << ", 2)";            
            sep = fsep;            
          }          
        }        
        key_ChannelInventoryEstimStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(users_regular), "          
          "SUM(users_from_now) "          
        "FROM ChannelInventoryEstimStats "        
        "WHERE ";        
      qstr << key_ChannelInventoryEstimStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.sdate_used() && !key_.sdate_is_null() &&
        key_.sdate() != Generics::Time::ZERO)      
        query.set(key_.sdate().get_gm_time().get_date());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.channel_id_used() && !key_.channel_id_is_null())      
        query.set(key_.channel_id());        
      if (key_.match_level_used() && !key_.match_level_is_null())      
        query.set(key_.match_level());        
      return ask(query);      
    }    
  }  
}
