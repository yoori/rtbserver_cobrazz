
#include "TagRequestGroupProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<TagRequestGroupProfileAdmin, 9>::names_type const  
  BaseAdminCmd<TagRequestGroupProfileAdmin, 9>::field_names = {  
    "country",    
    "colo_id",    
    "site_id",    
    "page_load_id",    
    "referer_hash",    
    "min_time",    
    "max_time",    
    "tags",    
    "ad_shown"    
  };  
  bool  
  TagRequestGroupProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  TagRequestGroupProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
