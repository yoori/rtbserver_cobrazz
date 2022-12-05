
#include "ColocationAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<ColocationAdmin, 4>::names_type const  
  BaseAdminCmd<ColocationAdmin, 4>::field_names = {  
    "colo_id",    
    "colo_rate_id",    
    "account_id",    
    "revenue_share"    
  };  
  bool  
  ColocationAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  ColocationAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
