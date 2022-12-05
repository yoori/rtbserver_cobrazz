
#include "RequestProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<RequestProfileAdmin, 1>::names_type const  
  BaseAdminCmd<RequestProfileAdmin, 1>::field_names = {  
    "fraud"    
  };  
  bool  
  RequestProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  RequestProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
