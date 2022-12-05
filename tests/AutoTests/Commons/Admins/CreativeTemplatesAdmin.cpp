
#include "CreativeTemplatesAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<CreativeTemplatesAdmin, 8>::names_type const  
  BaseAdminCmd<CreativeTemplatesAdmin, 8>::field_names = {  
    "creative_format",    
    "creative_size",    
    "app_format",    
    "track_impression",    
    "template_file",    
    "type",    
    "timestamp",    
    "status"    
  };  
  bool  
  CreativeTemplatesAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  CreativeTemplatesAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
