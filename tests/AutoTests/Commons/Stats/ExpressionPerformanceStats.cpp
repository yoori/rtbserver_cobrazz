
#include "ExpressionPerformanceStats.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ExpressionPerformanceStats    
    template<>    
    BasicStats<ExpressionPerformanceStats, 3>::names_type const    
    BasicStats<ExpressionPerformanceStats, 3>::field_names = {    
      ".imps",      
      ".clicks",      
      ".actions"      
    };    
    void    
    ExpressionPerformanceStats::print_idname (std::ostream& out) const    
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
      if (key_.expression_used())      
      {      
        out << sep; 
        out << "expression = '";
        if (key_.expression_is_null())
          out << "null'";
        else
          out << key_.expression() << '\'';        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    ExpressionPerformanceStats::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_ExpressionPerformanceStats;      
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
        if (key_.expression_used())        
        {        
          if (key_.expression_is_null())          
          {          
            kstr << sep << "expression is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "expression = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_ExpressionPerformanceStats = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(imps_verified), "          
          "SUM(clicks), "          
          "SUM(actions) "          
        "FROM ExpressionPerformance "        
        "WHERE ";        
      qstr << key_ExpressionPerformanceStats;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.sdate_used() && !key_.sdate_is_null() &&
        key_.sdate() != Generics::Time::ZERO)      
        query.set(key_.sdate().get_gm_time().get_date());        
      if (key_.cc_id_used() && !key_.cc_id_is_null())      
        query.set(key_.cc_id());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.expression_used() && !key_.expression_is_null())      
        query.set(key_.expression());        
      return ask(query);      
    }    
  }  
}
