
#include "FreqCapAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<FreqCapAdmin, 3>::names_type const  
  BaseAdminCmd<FreqCapAdmin, 3>::field_names = {  
    "id",    
    "window_time",    
    "window_limit"    
  };  
  bool  
  FreqCapAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  FreqCapAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
