
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include "SiteChecker.hpp"

namespace AutoTest
{
  bool  
  SiteChecker::check(bool throw_error)  
    /*throw(CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<SiteAdmin, CT_ALL> admins;    
        
    admins.initialize(    
      test_,    
      CTE_ALL,    
      STE_CAMPAIGN_MANAGER,    
      site_);    
        
    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    
        
  }  
}
