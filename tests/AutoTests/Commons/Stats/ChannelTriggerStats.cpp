
#include "ChannelTriggerStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelTriggerStats    
    template<>    
    BasicStats<ChannelTriggerStats, 3>::names_type const    
    BasicStats<ChannelTriggerStats, 3>::field_names = {    
      ".hits",      
      ".imps",      
      ".clicks"      
    };    
    void    
    ChannelTriggerStats::print_idname (std::ostream& out) const    
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
      if (key_.channel_trigger_id_used())      
      {      
        out << sep; 
        out << "channel_trigger_id = ";
        if (key_.channel_trigger_id_is_null())
          out << "null";
        else
          out << key_.channel_trigger_id();        
        sep = fsep;        
      }      
      if (key_.trigger_type_used())      
      {      
        out << sep; 
        out << "trigger_type = '";
        if (key_.trigger_type_is_null())
          out << "null'";
        else
          out << key_.trigger_type() << '\'';        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    ChannelTriggerStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ChannelTriggerStats;      
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
        if (key_.channel_trigger_id_used())        
        {        
          if (key_.channel_trigger_id_is_null())          
          {          
            kstr << sep << "channel_trigger_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "channel_trigger_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.trigger_type_used())        
        {        
          if (key_.trigger_type_is_null())          
          {          
            kstr << sep << "trigger_type is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "trigger_type = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_ChannelTriggerStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(hits), "          
          "SUM(approximated_imps), "          
          "SUM(approximated_clicks) "          
        "FROM ChannelTriggerStats "        
        "WHERE ";        
      qstr << key_ChannelTriggerStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.sdate_used() && !key_.sdate_is_null() &&
        key_.sdate() != Generics::Time::ZERO)      
        query.set(key_.sdate().get_gm_time().get_date());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.channel_id_used() && !key_.channel_id_is_null())      
        query.set(key_.channel_id());        
      if (key_.channel_trigger_id_used() && !key_.channel_trigger_id_is_null())      
        query.set(key_.channel_trigger_id());        
      if (key_.trigger_type_used() && !key_.trigger_type_is_null())      
        query.set(key_.trigger_type());        
      return ask(query);      
    }    
  }  
}
