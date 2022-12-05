
#include "ActionProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<ActionProfileAdmin, 5>::names_type const  
  BaseAdminCmd<ActionProfileAdmin, 5>::field_names = {  
    "action_markers",    
    "wait_markers",    
    "custom_action_markers",    
    "custom_wait_actions",    
    "custom_done_actions"    
  };  
  bool  
  ActionProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  ActionProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
