
#include "WebStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// WebStats    
    template<>    
    BasicStats<WebStats, 1>::names_type const    
    BasicStats<WebStats, 1>::field_names = {    
      ".count"      
    };    
    void    
    WebStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
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
      if (key_.ct_used())      
      {      
        out << sep; 
        out << "ct = '";
        if (key_.ct_is_null())
          out << "null'";
        else
          out << key_.ct() << '\'';        
        sep = fsep;        
      }      
      if (key_.curct_used())      
      {      
        out << sep; 
        out << "curct = '";
        if (key_.curct_is_null())
          out << "null'";
        else
          out << key_.curct() << '\'';        
        sep = fsep;        
      }      
      if (key_.browser_used())      
      {      
        out << sep; 
        out << "browser = '";
        if (key_.browser_is_null())
          out << "null'";
        else
          out << key_.browser() << '\'';        
        sep = fsep;        
      }      
      if (key_.os_used())      
      {      
        out << sep; 
        out << "os = '";
        if (key_.os_is_null())
          out << "null'";
        else
          out << key_.os() << '\'';        
        sep = fsep;        
      }      
      if (key_.app_used())      
      {      
        out << sep; 
        out << "app = '";
        if (key_.app_is_null())
          out << "null'";
        else
          out << key_.app() << '\'';        
        sep = fsep;        
      }      
      if (key_.source_used())      
      {      
        out << sep; 
        out << "source = '";
        if (key_.source_is_null())
          out << "null'";
        else
          out << key_.source() << '\'';        
        sep = fsep;        
      }      
      if (key_.operation_used())      
      {      
        out << sep; 
        out << "operation = '";
        if (key_.operation_is_null())
          out << "null'";
        else
          out << key_.operation() << '\'';        
        sep = fsep;        
      }      
      if (key_.result_used())      
      {      
        out << sep; 
        out << "result = '";
        if (key_.result_is_null())
          out << "null'";
        else
          out << key_.result() << '\'';        
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
      if (key_.test_used())      
      {      
        out << sep; 
        out << "test = ";
        if (key_.test_is_null())
          out << "null";
        else
          out << (key_.test() ? "true" : "false");        
        sep = fsep;        
      }      
      if (key_.cc_id_used())      
      {      
        out << sep; 
        out << "cc_id = ";
        if (key_.cc_id_is_null())
          out << "null";
        else
          out << key_.cc_id();        
        sep = fsep;        
      }      
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
      out << " >" << std::endl;      
    }    
    bool    
    WebStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_WebStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
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
        if (key_.ct_used())        
        {        
          if (key_.ct_is_null())          
          {          
            kstr << sep << "ct is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "ct = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.curct_used())        
        {        
          if (key_.curct_is_null())          
          {          
            kstr << sep << "curct is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "curct = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.browser_used())        
        {        
          if (key_.browser_is_null())          
          {          
            kstr << sep << "upb.value is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "upb.value = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.os_used())        
        {        
          if (key_.os_is_null())          
          {          
            kstr << sep << "upo.value is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "upo.value = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.app_used())        
        {        
          if (key_.app_is_null())          
          {          
            kstr << sep << "wbo.app is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "wbo.app = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.source_used())        
        {        
          if (key_.source_is_null())          
          {          
            kstr << sep << "wbo.source is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "wbo.source = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.operation_used())        
        {        
          if (key_.operation_is_null())          
          {          
            kstr << sep << "wbo.operation is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "wbo.operation = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.result_used())        
        {        
          if (key_.result_is_null())          
          {          
            kstr << sep << "result is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "result = :" << ckey++;            
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
        if (key_.test_used())        
        {        
          if (key_.test_is_null())          
          {          
            kstr << sep << "test is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "test = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.cc_id_used())        
        {        
          if (key_.cc_id_is_null())          
          {          
            kstr << sep << "cc_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "cc_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
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
        key_WebStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(count) "          
        "FROM WebStats stat "        
        "  Left join weboperation wbo on (wbo.web_operation_id = stat.web_operation_id) "        
        "  Left join userproperty upo on (upo.name = 'OsVersion' and stat.os_property_id = upo.user_property_id) "        
        "  Left join userproperty upb on (upb.name = 'BrowserVersion' and stat.browser_property_id = upb.user_property_id) "        
        "WHERE ";        
      qstr << key_WebStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.stimestamp_used() && !key_.stimestamp_is_null() &&
        key_.stimestamp() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.stimestamp().get_gm_time()));        
      if (key_.country_sdate_used() && !key_.country_sdate_is_null() &&
        key_.country_sdate() != Generics::Time::ZERO)      
        query.set(key_.country_sdate().get_gm_time().get_date());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.ct_used() && !key_.ct_is_null())      
        query.set(key_.ct());        
      if (key_.curct_used() && !key_.curct_is_null())      
        query.set(key_.curct());        
      if (key_.browser_used() && !key_.browser_is_null())      
        query.set(key_.browser());        
      if (key_.os_used() && !key_.os_is_null())      
        query.set(key_.os());        
      if (key_.app_used() && !key_.app_is_null())      
        query.set(key_.app());        
      if (key_.source_used() && !key_.source_is_null())      
        query.set(key_.source());        
      if (key_.operation_used() && !key_.operation_is_null())      
        query.set(key_.operation());        
      if (key_.result_used() && !key_.result_is_null())      
        query.set(key_.result());        
      if (key_.user_status_used() && !key_.user_status_is_null())      
        query.set(key_.user_status());        
      if (key_.test_used() && !key_.test_is_null())      
        query.set(key_.test());        
      if (key_.cc_id_used() && !key_.cc_id_is_null())      
        query.set(key_.cc_id());        
      if (key_.tag_id_used() && !key_.tag_id_is_null())      
        query.set(key_.tag_id());        
      return ask(query);      
    }    
  }  
}
