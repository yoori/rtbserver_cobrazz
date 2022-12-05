
#include "PublisherStatsDaily.hpp"

namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// PublisherStatsDaily    
    template<>    
    BasicStats<PublisherStatsDaily, 7>::names_type const    
    BasicStats<PublisherStatsDaily, 7>::field_names = {    
      ".requests",      
      ".imps",      
      ".clicks",      
      ".actions",      
      ".pub_amount",      
      ".pub_comm_amount",      
      ".pub_credited_imps"      
    };    
    void    
    PublisherStatsDaily::print_idname (std::ostream& out) const    
    {    
      const char* sep = " ";      
      static const char* fsep = ", ";      
      out << description_ << " <";      
      if (key_.pub_sdate_used())      
      {      
        out << sep; 
        out << "pub_sdate = '";
        if (key_.pub_sdate_is_null())
          out << "null";
        else
          out <<  key_.pub_sdate().get_gm_time().format("%Y-%m-%d") << '\'';        
        sep = fsep;        
      }      
      if (key_.pub_account_id_used())      
      {      
        out << sep; 
        out << "pub_account_id = ";
        if (key_.pub_account_id_is_null())
          out << "null";
        else
          out << key_.pub_account_id();        
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
      if (key_.size_id_used())      
      {      
        out << sep; 
        out << "size_id = ";
        if (key_.size_id_is_null())
          out << "null";
        else
          out << key_.size_id();        
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
      if (key_.num_shown_used())      
      {      
        out << sep; 
        out << "num_shown = ";
        if (key_.num_shown_is_null())
          out << "null";
        else
          out << key_.num_shown();        
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
      if (key_.walled_garden_used())      
      {      
        out << sep; 
        out << "walled_garden = ";
        if (key_.walled_garden_is_null())
          out << "null";
        else
          out << (key_.walled_garden() ? "true" : "false");        
        sep = fsep;        
      }      
      out << " >" << std::endl;      
    }    
    bool    
    PublisherStatsDaily::query_select_(StatsDB::IConn& connection)    
    {    
      std::string key_PublisherStatsDaily;      
      {      
        std::ostringstream kstr;        
        int ckey = 1;        
        const char* sep = "";        
        static const char* fsep = " and ";        
        if (key_.pub_sdate_used())        
        {        
          if (key_.pub_sdate_is_null())          
          {          
            kstr << sep << "pub_sdate is null";            
            sep = fsep;            
          }          
          else if (key_.pub_sdate() == Generics::Time::ZERO)          
          {          
            kstr << sep << "pub_sdate = '-infinity'";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "pub_sdate = :"<< ckey++;            
            sep = fsep;            
          }          
        }        
        if (key_.pub_account_id_used())        
        {        
          if (key_.pub_account_id_is_null())          
          {          
            kstr << sep << "pub_account_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "pub_account_id = :" << ckey++;            
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
        if (key_.size_id_used())        
        {        
          if (key_.size_id_is_null())          
          {          
            kstr << sep << "size_id is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "size_id = :" << ckey++;            
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
        if (key_.num_shown_used())        
        {        
          if (key_.num_shown_is_null())          
          {          
            kstr << sep << "num_shown is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "num_shown = :" << ckey++;            
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
        if (key_.walled_garden_used())        
        {        
          if (key_.walled_garden_is_null())          
          {          
            kstr << sep << "walled_garden is null";            
            sep = fsep;            
          }          
          else          
          {          
            kstr << sep << "walled_garden = :" << ckey++;            
            sep = fsep;            
          }          
        }        
        key_PublisherStatsDaily = kstr.str();        
      }      
      std::ostringstream qstr;      
      qstr <<      
        "SELECT "        
          "SUM(requests), "          
          "SUM(imps), "          
          "SUM(clicks), "          
          "SUM(actions), "          
          "SUM(pub_amount), "          
          "SUM(pub_comm_amount), "          
          "SUM(pub_credited_imps) "          
        "FROM PublisherStatsDaily "        
        "WHERE ";        
      qstr << key_PublisherStatsDaily;      
      StatsDB::Query query(connection.get_query(qstr.str()));      
      if (key_.pub_sdate_used() && !key_.pub_sdate_is_null() &&
        key_.pub_sdate() != Generics::Time::ZERO)      
        query.set(key_.pub_sdate().get_gm_time().get_date());        
      if (key_.pub_account_id_used() && !key_.pub_account_id_is_null())      
        query.set(key_.pub_account_id());        
      if (key_.site_id_used() && !key_.site_id_is_null())      
        query.set(key_.site_id());        
      if (key_.size_id_used() && !key_.size_id_is_null())      
        query.set(key_.size_id());        
      if (key_.tag_id_used() && !key_.tag_id_is_null())      
        query.set(key_.tag_id());        
      if (key_.num_shown_used() && !key_.num_shown_is_null())      
        query.set(key_.num_shown());        
      if (key_.country_code_used() && !key_.country_code_is_null())      
        query.set(key_.country_code());        
      if (key_.walled_garden_used() && !key_.walled_garden_is_null())      
        query.set(key_.walled_garden());        
      return ask(query);      
    }    
  }  
}
