
#include "TriggerAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<TriggerAdmin, 10>::names_type const  
  BaseAdminCmd<TriggerAdmin, 10>::field_names = {  
    "channel_id",    
    "url",    
    "neg_url",    
    "page_word",    
    "neg_page_word",    
    "search_word",    
    "neg_search_word",    
    "url_keyword",    
    "neg_url_keyword",    
    "stamp"    
  };  
  bool  
  TriggerAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  TriggerAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
