
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>

namespace AutoTest
{
  template<typename ExpectedType>  
  bool  
  StatAccountChecker_<ExpectedType>::check(bool throw_error)  
    /*throw (CheckFailed, eh::Exception)*/  
  {  
 
    AdminsArray<StatAccountAdmin, CT_ALL> admins;    

    admins.initialize(    
      test_,    
      CTE_ALL,    
      srv_type_by_index(    
        static_cast<size_t>(CampaignServer)),    
      id_    
    );    

    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    

  }  
}
