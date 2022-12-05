
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>

namespace AutoTest
{
  template<typename ExpectedType>  
  bool  
  ChannelSearchChecker_<ExpectedType>::check(bool throw_error)  
    /*throw (CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<ChannelSearchAdmin, CT_ALL> admins;    

    admins.initialize(    
      test_,    
      CTE_ALL,    
      srv_type_by_index(    
        static_cast<size_t>(service_)),    
      phrase_,    
      service_    
    );    

    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    

  }  
}
