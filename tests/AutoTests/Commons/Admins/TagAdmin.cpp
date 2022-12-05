
#include "TagAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<TagAdmin, 6>::names_type const  
  BaseAdminCmd<TagAdmin, 6>::field_names = {  
    "tag_id",    
    "site_id",    
    "sizes",    
    "accepted_categories",    
    "rejected_categories",    
    "imp_track_pixel"    
  };  
  bool  
  TagAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  TagAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
