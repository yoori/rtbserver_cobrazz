
#include "HistoryProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<HistoryProfileAdmin, 3>::names_type const  
  BaseAdminCmd<HistoryProfileAdmin, 3>::field_names = {  
    "page_channels",    
    "search_channels",    
    "url_channels"    
  };  
  bool  
  HistoryProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  HistoryProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
