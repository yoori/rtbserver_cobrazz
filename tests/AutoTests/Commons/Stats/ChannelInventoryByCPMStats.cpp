
#include "ChannelInventoryByCPMStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelInventoryByCPMStats    
    template<>    
    BasicStats<ChannelInventoryByCPMStats, 2>::names_type const    
    BasicStats<ChannelInventoryByCPMStats, 2>::field_names = {    
      ".user_count",      
      ".impops"      
    };    
    void    
    ChannelInventoryByCPMStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
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
      if (key_.creative_size_id_used())      
      {      
        out << sep; 
        out << "creative_size_id = ";
        if (key_.creative_size_id_is_null())
          out << "null";
        else
          out << key_.creative_size_id();        
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
      if (key_.ecpm_used())      
      {      
        out << sep; 
        out << "ecpm = ";
        if (key_.ecpm_is_null())
          out << "null";
        else
          out << key_.ecpm();        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    ChannelInventoryByCPMStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ChannelInventoryByCPMStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
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
        if (key_.creative_size_id_used())        
        {        
          if (key_.creative_size_id_is_null())          
          {          
            kstr << sep << "creative_size_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "creative_size_id = :" << ckey++;            
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
        if (key_.ecpm_used())        
        {        
          if (key_.ecpm_is_null())          
          {          
            kstr << sep << "ecpm is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "TRUNC(ecpm, 2) = TRUNC(:" << ckey++ << ", 2)";            
            sep = fsep;            
          }          
        }        
        key_ChannelInventoryByCPMStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(user_count), "          
          "SUM(impops) "          
        "FROM ChannelInventoryByCpm "        
        "WHERE ";        
      qstr << key_ChannelInventoryByCPMStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.channel_id_used() && !key_.channel_id_is_null())      
        query.set(key_.channel_id());        
      if (key_.sdate_used() && !key_.sdate_is_null() &&
        key_.sdate() != Generics::Time::ZERO)      
        query.set(key_.sdate().get_gm_time().get_date());        
      if (key_.creative_size_id_used() && !key_.creative_size_id_is_null())      
        query.set(key_.creative_size_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.country_code_used() && !key_.country_code_is_null())      
        query.set(key_.country_code());        
      if (key_.ecpm_used() && !key_.ecpm_is_null())      
        query.set(key_.ecpm());        
      return ask(query);      
    }    
  }  
}
