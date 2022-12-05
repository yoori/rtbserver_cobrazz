
#include "SearchEngineAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<SearchEngineAdmin, 5>::names_type const  
  BaseAdminCmd<SearchEngineAdmin, 5>::field_names = {  
    "id",    
    "regexp",    
    "encoding",    
    "decoding_depth",    
    "timestamp"    
  };  
  bool  
  SearchEngineAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  SearchEngineAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
