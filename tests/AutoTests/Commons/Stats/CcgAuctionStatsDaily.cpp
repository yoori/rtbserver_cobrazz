
#include "CcgAuctionStatsDaily.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// CcgAuctionStatsDaily    
    template<>    
    BasicStats<CcgAuctionStatsDaily, 1>::names_type const    
    BasicStats<CcgAuctionStatsDaily, 1>::field_names = {    
      ".auctions_lost"      
    };    
    void    
    CcgAuctionStatsDaily::print_idname (std::ostream& out) const    
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
      if (key_.ccg_id_used())      
      {      
        out << sep; 
        out << "ccg_id = ";
        if (key_.ccg_id_is_null())
          out << "null";
        else
          out << key_.ccg_id();        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    CcgAuctionStatsDaily::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_CcgAuctionStatsDaily;      
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
        if (key_.ccg_id_used())        
        {        
          if (key_.ccg_id_is_null())          
          {          
            kstr << sep << "ccg_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "ccg_id = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_CcgAuctionStatsDaily = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(auctions_lost) "          
        "FROM CcgAuctionStatsDaily "        
        "WHERE ";        
      qstr << key_CcgAuctionStatsDaily;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.adv_sdate_used() && !key_.adv_sdate_is_null() &&
        key_.adv_sdate() != Generics::Time::ZERO)      
        query.set(key_.adv_sdate().get_gm_time().get_date());        
      if (key_.colo_id_used() && !key_.colo_id_is_null())      
        query.set(key_.colo_id());        
      if (key_.ccg_id_used() && !key_.ccg_id_is_null())      
        query.set(key_.ccg_id());        
      return ask(query);      
    }    
  }  
}
