
#include "InventoryProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<InventoryProfileAdmin, 4>::names_type const  
  BaseAdminCmd<InventoryProfileAdmin, 4>::field_names = {  
    "user_id",    
    "imp_count",    
    "total_channels",    
    "last_request_time"    
  };  
  bool  
  InventoryProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  InventoryProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
