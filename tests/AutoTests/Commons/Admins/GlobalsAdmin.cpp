
#include "GlobalsAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<GlobalsAdmin, 5>::names_type const  
  BaseAdminCmd<GlobalsAdmin, 5>::field_names = {  
    "currency_exchange_id",    
    "foros_min_margin",    
    "foros_min_fixed_margin",    
    "global_params_timestamp",    
    "master_stamp"    
  };  
  bool  
  GlobalsAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  GlobalsAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
