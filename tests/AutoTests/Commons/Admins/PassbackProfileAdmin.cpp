
#include "PassbackProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<PassbackProfileAdmin, 6>::names_type const  
  BaseAdminCmd<PassbackProfileAdmin, 6>::field_names = {  
    "tag_id",    
    "colo_id",    
    "user_status",    
    "time",    
    "done",    
    "verified"    
  };  
  bool  
  PassbackProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  PassbackProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
