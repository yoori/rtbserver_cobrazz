
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include "TagChecker.hpp"

namespace AutoTest
{
  bool  
  TagChecker::check(bool throw_error)  
    /*throw(CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<TagAdmin, CT_ALL> admins;    
        
    admins.initialize(    
      test_,    
      CTE_ALL,    
      STE_CAMPAIGN_MANAGER,    
      tag_);    
        
    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    
        
  }  
}
