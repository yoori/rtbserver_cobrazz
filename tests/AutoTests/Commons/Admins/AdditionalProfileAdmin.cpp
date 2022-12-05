
#include "AdditionalProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<AdditionalProfileAdmin, 19>::names_type const  
  BaseAdminCmd<AdditionalProfileAdmin, 19>::field_names = {  
    "version",    
    "create_time",    
    "history_time",    
    "ignore_fraud_time",    
    "last_request_time",    
    "session_start_time",    
    "colo_timestamps",    
    "page_ht_candidates",    
    "page_history_matches",    
    "page_history_visits",    
    "page_session_matches",    
    "search_ht_candidates",    
    "search_history_matches",    
    "search_history_visits",    
    "search_session_matches",    
    "url_ht_candidates",    
    "url_history_matches",    
    "url_history_visits",    
    "url_session_matches"    
  };  
  bool  
  AdditionalProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  AdditionalProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
