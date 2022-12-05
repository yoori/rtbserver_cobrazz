
#include "CCGKeywordStatsDaily.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// CCGKeywordStatsDaily    
    template<>    
    BasicStats<CCGKeywordStatsDaily, 5>::names_type const    
    BasicStats<CCGKeywordStatsDaily, 5>::field_names = {    
      ".imps",      
      ".clicks",      
      ".adv_amount",      
      ".adv_comm_amount",      
      ".pub_amount_adv"      
    };    
    void    
    CCGKeywordStatsDaily::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.adv_sdate_used())      
      {      
        out << sep; 
        out << "adv_sdate = '";
        if (key_.adv_sdate_is_null())
          out << "null";
        else
          out <<  key_.adv_sdate().get_gm_time().format("%Y-%m-%d") << '\'';        
        sep = fsep;        
      }      
      if (key_.ccg_keyword_id_used())      
      {      
        out << sep; 
        out << "ccg_keyword_id = ";
        if (key_.ccg_keyword_id_is_null())
          out << "null";
        else
          out << key_.ccg_keyword_id();        
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
      out << " >" << std::endl;      
    }    
    bool    
    CCGKeywordStatsDaily::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_CCGKeywordStatsDaily;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.adv_sdate_used())        
        {        
          if (key_.adv_sdate_is_null())          
          {          
            kstr << sep << "adv_sdate is null";            
            sep = fsep;            
          }          
          else if (key_.adv_sdate() == Generics::Time::ZERO)          
          {          
            kstr << sep << "adv_sdate = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "adv_sdate = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.ccg_keyword_id_used())        
        {        
          if (key_.ccg_keyword_id_is_null())          
          {          
            kstr << sep << "ccg_keyword_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "ccg_keyword_id = :" << ckey++;            
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
        key_CCGKeywordStatsDaily = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(imps), "          
          "SUM(clicks), "          
          "SUM(adv_amount), "          
          "SUM(adv_comm_amount), "          
          "SUM(pub_amount_adv) "          
        "FROM CCGKeywordStatsDaily "        
        "WHERE ";        
      qstr << key_CCGKeywordStatsDaily;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.adv_sdate_used() && !key_.adv_sdate_is_null() &&
        key_.adv_sdate() != Generics::Time::ZERO)      
        query.set(key_.adv_sdate().get_gm_time().get_date());        
      if (key_.ccg_keyword_id_used() && !key_.ccg_keyword_id_is_null())      
        query.set(key_.ccg_keyword_id());        
      if (key_.cc_id_used() && !key_.cc_id_is_null())      
        query.set(key_.cc_id());        
      return ask(query);      
    }    
  }  
}
