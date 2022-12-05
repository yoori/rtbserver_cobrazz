
#include "ChannelSearchAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<ChannelSearchAdmin, 1>::names_type const  
  BaseAdminCmd<ChannelSearchAdmin, 1>::field_names = {  
    "channel_id"    
  };  
  bool  
  ChannelSearchAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  ChannelSearchAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
