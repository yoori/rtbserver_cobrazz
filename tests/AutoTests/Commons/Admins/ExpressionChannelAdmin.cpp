
#include "ExpressionChannelAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<ExpressionChannelAdmin, 6>::names_type const  
  BaseAdminCmd<ExpressionChannelAdmin, 6>::field_names = {  
    "channel_id",    
    "discover_query",    
    "discover_annotation",    
    "expression",    
    "imp_revenue",    
    "status"    
  };  
  bool  
  ExpressionChannelAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  ExpressionChannelAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
