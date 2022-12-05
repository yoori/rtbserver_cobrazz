
#include "ActionAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<ActionAdmin, 3>::names_type const  
  BaseAdminCmd<ActionAdmin, 3>::field_names = {  
    "action_id",    
    "timestamp",    
    "ccg_ids"    
  };  
  bool  
  ActionAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  ActionAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
