
#include "ChannelCheckAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<ChannelCheckAdmin, 2>::names_type const  
  BaseAdminCmd<ChannelCheckAdmin, 2>::field_names = {  
    "id",    
    "version"    
  };  
  bool  
  ChannelCheckAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  ChannelCheckAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
