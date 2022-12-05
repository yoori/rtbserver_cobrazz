
#include "TagIdBasedStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// TagIdBasedStats    
    template<>    
    BasicStats<TagIdBasedStats, 4>::names_type const    
    BasicStats<TagIdBasedStats, 4>::field_names = {    
      ".TagAuctionStats.requests",      
      ".PublisherInventory.imps",      
      ".PublisherInventory.requests",      
      ".PublisherInventory.revenue"      
    };    
    void    
    TagIdBasedStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.tag_id_used())      
      {      
        out << sep; 
        out << "tag_id = ";
        if (key_.tag_id_is_null())
          out << "null";
        else
          out << key_.tag_id();        
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
    TagIdBasedStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_TagIdBasedStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.tag_id_used())        
        {        
          if (key_.tag_id_is_null())          
          {          
            kstr << sep << "tag_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "tag_id = :" << ckey++;            
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
        key_TagIdBasedStats = kstr.str();        
      }      
      std::string key_PublisherInventory;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.tag_id_used())        
        {        
          if (key_.tag_id_is_null())          
          {          
            kstr << sep << "tag_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "tag_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_PublisherInventory = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT * FROM ";        
          qstr << "("          
          "SELECT "          
            "SUM(requests) "            
          "FROM TagAuctionStats "          
          "WHERE ";          
          qstr << key_TagIdBasedStats << ") AS TagAuctionStats,";          
          qstr << "("          
          "SELECT "          
            "SUM(imps), "            
            "SUM(requests), "            
            "SUM(revenue) "            
          "FROM PublisherInventory "          
          "WHERE ";          
          qstr << key_PublisherInventory << ") AS PublisherInventory";          
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.tag_id_used() && !key_.tag_id_is_null())      
        query.set(key_.tag_id());        
      if (key_.exclude_colo_used() && !key_.exclude_colo_is_null())      
        query.set(key_.exclude_colo());        
      if (key_.tag_id_used() && !key_.tag_id_is_null())      
        query.set(key_.tag_id());        
      return ask(query);      
    }    
  }  
}
