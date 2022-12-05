
#include "StatAccountAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<StatAccountAdmin, 5>::names_type const  
  BaseAdminCmd<StatAccountAdmin, 5>::field_names = {  
    "account_id",    
    "amount",    
    "comm_amount",    
    "daily_amount",    
    "daily_comm_amount"    
  };  
  bool  
  StatAccountAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  StatAccountAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
