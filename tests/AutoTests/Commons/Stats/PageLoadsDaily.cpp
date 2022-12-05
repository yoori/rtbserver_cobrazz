
#include "PageLoadsDaily.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// PageLoadsDaily    
    template<>    
    BasicStats<PageLoadsDaily, 6>::names_type const    
    BasicStats<PageLoadsDaily, 6>::field_names = {    
      ".page_loads",      
      ".page_loads_min",      
      ".page_loads_max",      
      ".utilized_page_loads",      
      ".utilized_page_loads_min",      
      ".utilized_page_loads_max"      
    };    
    void    
    PageLoadsDaily::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.country_sdate_used())      
      {      
        out << sep; 
        out << "country_sdate = '";
        if (key_.country_sdate_is_null())
          out << "null";
        else
          out <<  key_.country_sdate().get_gm_time().format("%Y-%m-%d") << '\'';        
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
      if (key_.tag_group_used())      
      {      
        out << sep; 
        out << "tag_group = '";
        if (key_.tag_group_is_null())
          out << "null'";
        else
          out << key_.tag_group() << '\'';        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    PageLoadsDaily::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_PageLoadsDaily;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.country_sdate_used())        
        {        
          if (key_.country_sdate_is_null())          
          {          
            kstr << sep << "country_sdate is null";            
            sep = fsep;            
          }          
          else if (key_.country_sdate() == Generics::Time::ZERO)          
          {          
            kstr << sep << "country_sdate = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "country_sdate = :"<< ckey++;            
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
        if (key_.tag_group_used())        
        {        
          if (key_.tag_group_is_null())          
          {          
            kstr << sep << "tag_group is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "tag_group = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_PageLoadsDaily = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(page_loads), "          
          "MIN(page_loads), "          
          "MAX(page_loads), "          
          "SUM(utilized_page_loads), "          
          "MIN(utilized_page_loads), "          
          "MAX(utilized_page_loads) "          
        "FROM PageLoadsDaily "        
        "WHERE ";        
      qstr << key_PageLoadsDaily;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.country_sdate_used() && !key_.country_sdate_is_null() &&
        key_.country_sdate() != Generics::Time::ZERO)      
        query.set(key_.country_sdate().get_gm_time().get_date());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.site_id_used() && !key_.site_id_is_null())      
        query.set(key_.site_id());        
      if (key_.country_code_used() && !key_.country_code_is_null())      
        query.set(key_.country_code());        
      if (key_.tag_group_used() && !key_.tag_group_is_null())      
        query.set(key_.tag_group());        
      return ask(query);      
    }    
  }  
}
