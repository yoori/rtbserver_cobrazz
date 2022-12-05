
#include "UserKeywordProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<UserKeywordProfileAdmin, 6>::names_type const  
  BaseAdminCmd<UserKeywordProfileAdmin, 6>::field_names = {  
    "page_matches",    
    "search_matches",    
    "ccg_imps",    
    "cc_imps",    
    "channel_imps",    
    "requests"    
  };  
  bool  
  UserKeywordProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  UserKeywordProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
