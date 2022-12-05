
#include "CreativeCategoryAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<CreativeCategoryAdmin, 2>::names_type const  
  BaseAdminCmd<CreativeCategoryAdmin, 2>::field_names = {  
    "creative_category_id",    
    "timestamp"    
  };  
  bool  
  CreativeCategoryAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  CreativeCategoryAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
