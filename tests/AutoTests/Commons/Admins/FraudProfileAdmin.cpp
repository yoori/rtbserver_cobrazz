
#include "FraudProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<FraudProfileAdmin, 1>::names_type const  
  BaseAdminCmd<FraudProfileAdmin, 1>::field_names = {  
    "fraud_time"    
  };  
  bool  
  FraudProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  FraudProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
