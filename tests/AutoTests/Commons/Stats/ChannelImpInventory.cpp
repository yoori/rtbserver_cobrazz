
#include "ChannelImpInventory.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelImpInventory    
    template<>    
    BasicStats<ChannelImpInventory, 13>::names_type const    
    BasicStats<ChannelImpInventory, 13>::field_names = {    
      ".imps",      
      ".clicks",      
      ".actions",      
      ".revenue",      
      ".impops_user_count",      
      ".imps_user_count",      
      ".imps_value",      
      ".imps_other",      
      ".imps_other_user_count",      
      ".imps_other_value",      
      ".impops_no_imp",      
      ".impops_no_imp_user_count",      
      ".impops_no_imp_value"      
    };    
    void    
    ChannelImpInventory::print_idname (std::ostream& out) const    
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
      if (key_.ccg_type_used())      
      {      
        out << sep; 
        out << "ccg_type = '";
        if (key_.ccg_type_is_null())
          out << "null'";
        else
          out << key_.ccg_type() << '\'';        
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
      out << " >" << std::endl;      
    }    
    bool    
    ChannelImpInventory::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ChannelImpInventory;      
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
        if (key_.ccg_type_used())        
        {        
          if (key_.ccg_type_is_null())          
          {          
            kstr << sep << "ccg_type is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "ccg_type = :" << ckey++;            
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
        key_ChannelImpInventory = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(imps), "          
          "SUM(clicks), "          
          "SUM(actions), "          
          "SUM(revenue), "          
          "SUM(impops_user_count), "          
          "SUM(imps_user_count), "          
          "SUM(imps_value), "          
          "SUM(imps_other), "          
          "SUM(imps_other_user_count), "          
          "SUM(imps_other_value), "          
          "SUM(impops_no_imp), "          
          "SUM(impops_no_imp_user_count), "          
          "SUM(impops_no_imp_value) "          
        "FROM ChannelImpInventory "        
        "WHERE ";        
      qstr << key_ChannelImpInventory;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.channel_id_used() && !key_.channel_id_is_null())      
        query.set(key_.channel_id());        
      if (key_.ccg_type_used() && !key_.ccg_type_is_null())      
        query.set(key_.ccg_type());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.sdate_used() && !key_.sdate_is_null() &&
        key_.sdate() != Generics::Time::ZERO)      
        query.set(key_.sdate().get_gm_time().get_date());        
      return ask(query);      
    }    
  }  
}
