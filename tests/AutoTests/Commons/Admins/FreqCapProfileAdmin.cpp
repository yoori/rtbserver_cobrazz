
#include "FreqCapProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<FreqCapProfileAdmin, 4>::names_type const  
  BaseAdminCmd<FreqCapProfileAdmin, 4>::field_names = {  
    "fc_id",    
    "virtual",    
    "total_impressions",    
    "last_impressions"    
  };  
  bool  
  FreqCapProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  FreqCapProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
