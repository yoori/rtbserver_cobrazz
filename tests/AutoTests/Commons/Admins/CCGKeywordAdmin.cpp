
#include "CCGKeywordAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<CCGKeywordAdmin, 7>::names_type const  
  BaseAdminCmd<CCGKeywordAdmin, 7>::field_names = {  
    "ccg_keyword_id",    
    "ccg_id",    
    "channel_id",    
    "max_cpc",    
    "ctr",    
    "click_url",    
    "original_keyword"    
  };  
  bool  
  CCGKeywordAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  CCGKeywordAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
