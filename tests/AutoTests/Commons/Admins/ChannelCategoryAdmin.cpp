
#include "ChannelCategoryAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<ChannelCategoryAdmin, 3>::names_type const  
  BaseAdminCmd<ChannelCategoryAdmin, 3>::field_names = {  
    "channel_id",    
    "parent_channel_id",    
    "timestamp"    
  };  
  bool  
  ChannelCategoryAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  ChannelCategoryAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
