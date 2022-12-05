
#include "OptOutStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// OptOutStats    
    template<>    
    BasicStats<OptOutStats, 1>::names_type const    
    BasicStats<OptOutStats, 1>::field_names = {    
      ".count"      
    };    
    void    
    OptOutStats::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.isp_sdate_used())      
      {      
        out << sep; 
        out << "isp_sdate = '";
        if (key_.isp_sdate_is_null())
          out << "null";
        else
          out <<  key_.isp_sdate().get_gm_time().format("%Y-%m-%d:%H") << '\'';        
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
      if (key_.referer_used())      
      {      
        out << sep; 
        out << "referer = '";
        if (key_.referer_is_null())
          out << "null'";
        else
          out << key_.referer() << '\'';        
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
      if (key_.status_used())      
      {      
        out << sep; 
        out << "status = ";
        if (key_.status_is_null())
          out << "null";
        else
          out << key_.status();        
        sep = fsep;        
      }      
      if (key_.test_used())      
      {      
        out << sep; 
        out << "test = '";
        if (key_.test_is_null())
          out << "null'";
        else
          out << key_.test() << '\'';        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    OptOutStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_OptOutStats;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
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
        if (key_.referer_used())        
        {        
          if (key_.referer_is_null())          
          {          
            kstr << sep << "referer is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "referer = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.operation_used())        
        {        
          if (key_.operation_is_null())          
          {          
            kstr << sep << "operation is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "operation = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.status_used())        
        {        
          if (key_.status_is_null())          
          {          
            kstr << sep << "status is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "status = :" << ckey++;            
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
        key_OptOutStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(count) "          
        "FROM OptOutStats "        
        "WHERE ";        
      qstr << key_OptOutStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.isp_sdate_used() && !key_.isp_sdate_is_null() &&
        key_.isp_sdate() != Generics::Time::ZERO)      
        query.set(trunc_hourly(key_.isp_sdate().get_gm_time()));        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.referer_used() && !key_.referer_is_null())      
        query.set(key_.referer());        
      if (key_.operation_used() && !key_.operation_is_null())      
        query.set(key_.operation());        
      if (key_.status_used() && !key_.status_is_null())      
        query.set(key_.status());        
      if (key_.test_used() && !key_.test_is_null())      
        query.set(key_.test());        
      return ask(query);      
    }    
  }  
}
