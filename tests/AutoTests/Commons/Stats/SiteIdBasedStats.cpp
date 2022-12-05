
#include "SiteIdBasedStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// SiteIdBasedStats    
    template<>    
    BasicStats<SiteIdBasedStats, 3>::names_type const    
    BasicStats<SiteIdBasedStats, 3>::field_names = {    
      ".SiteUserStats.unique_users",      
      ".PageLoadsDaily.page_loads",      
      ".PageLoadsDaily.utilized_page_loads"      
    };    
    void    
    SiteIdBasedStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.site_id_used())      
      {      
        out << sep; 
        out << "site_id = ";
        if (key_.site_id_is_null())
          out << "null";
        else
          out << key_.site_id();        
        sep = fsep;        
      }      
      if (key_.exclude_colo_used())      
      {      
        out << sep; 
        out << "exclude_colo = ";
        if (key_.exclude_colo_is_null())
          out << "null";
        else
          out << key_.exclude_colo();        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    SiteIdBasedStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_SiteIdBasedStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.site_id_used())        
        {        
          if (key_.site_id_is_null())          
          {          
            kstr << sep << "site_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "site_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.exclude_colo_used())        
        {        
          if (key_.exclude_colo_is_null())          
          {          
            kstr << sep << "colo_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "colo_id != :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_SiteIdBasedStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT * FROM ";        
          qstr << "("          
          "SELECT "          
            "SUM(unique_users) "            
          "FROM SiteUserStats "          
          "WHERE ";          
          qstr << key_SiteIdBasedStats << ") AS SiteUserStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(page_loads), "            
            "SUM(utilized_page_loads) "            
          "FROM PageLoadsDaily "          
          "WHERE ";          
          qstr << key_SiteIdBasedStats << ") AS PageLoadsDaily";          
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.site_id_used() && !key_.site_id_is_null())      
        query.set(key_.site_id());        
      if (key_.exclude_colo_used() && !key_.exclude_colo_is_null())      
        query.set(key_.exclude_colo());        
      if (key_.site_id_used() && !key_.site_id_is_null())      
        query.set(key_.site_id());        
      if (key_.exclude_colo_used() && !key_.exclude_colo_is_null())      
        query.set(key_.exclude_colo());        
      return ask(query);      
    }    
  }  
}
