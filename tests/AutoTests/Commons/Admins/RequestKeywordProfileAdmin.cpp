
#include "RequestKeywordProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<RequestKeywordProfileAdmin, 20>::names_type const  
  BaseAdminCmd<RequestKeywordProfileAdmin, 20>::field_names = {  
    "time",    
    "colo_id",    
    "tag_id",    
    "ccg_id",    
    "cc_id",    
    "channel_id",    
    "position",    
    "num_shown",    
    "page_last_match",    
    "page_hourly_matches",    
    "page_daily_matches",    
    "page_weekly_matches",    
    "search_last_match",    
    "search_hourly_matches",    
    "search_daily_matches",    
    "search_weekly_matches",    
    "ccg_imps",    
    "cc_imps",    
    "channel_imps",    
    "click_done"    
  };  
  bool  
  RequestKeywordProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  RequestKeywordProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
