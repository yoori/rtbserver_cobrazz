
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include "ActionChecker.hpp"

namespace AutoTest
{
  bool  
  ActionChecker::check(bool throw_error)  
    /*throw(CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<ActionAdmin, CT_ALL> admins;    
        
    admins.initialize(    
      test_,    
      CTE_ALL,    
      srv_type_by_index(    
        static_cast<size_t>(CampaignManager)),    
      action_    
    );    
        
    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    
        
  }  
}
