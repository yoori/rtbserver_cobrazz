
#include "ChannelCountStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelCountStats    
    template<>    
    BasicStats<ChannelCountStats, 1>::names_type const    
    BasicStats<ChannelCountStats, 1>::field_names = {    
      ".users_count"      
    };    
    void    
    ChannelCountStats::print_idname (std::ostream& out) const    
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
      if (key_.channel_type_used())      
      {      
        out << sep; 
        out << "channel_type = '";
        if (key_.channel_type_is_null())
          out << "null'";
        else
          out << key_.channel_type() << '\'';        
        sep = fsep;        
      }      
      if (key_.channel_count_used())      
      {      
        out << sep; 
        out << "channel_count = ";
        if (key_.channel_count_is_null())
          out << "null";
        else
          out << key_.channel_count();        
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
    ChannelCountStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ChannelCountStats;      
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
        if (key_.channel_type_used())        
        {        
          if (key_.channel_type_is_null())          
          {          
            kstr << sep << "channel_type is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "channel_type = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.channel_count_used())        
        {        
          if (key_.channel_count_is_null())          
          {          
            kstr << sep << "channel_count is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "channel_count = :" << ckey++;            
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
        key_ChannelCountStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(users_count) "          
        "FROM ChannelCountStats "        
        "WHERE ";        
      qstr << key_ChannelCountStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.channel_type_used() && !key_.channel_type_is_null())      
        query.set(key_.channel_type());        
      if (key_.channel_count_used() && !key_.channel_count_is_null())      
        query.set(key_.channel_count());        
      if (key_.isp_sdate_used() && !key_.isp_sdate_is_null() &&
        key_.isp_sdate() != Generics::Time::ZERO)      
        query.set(key_.isp_sdate().get_gm_time().get_date());        
      return ask(query);      
    }    
  }  
}
