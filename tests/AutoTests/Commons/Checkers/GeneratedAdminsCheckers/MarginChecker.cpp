
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include "MarginChecker.hpp"

namespace AutoTest
{
  bool  
  MarginChecker::check(bool throw_error)  
    /*throw(CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<MarginAdmin, CT_ALL> admins;    
        
    admins.initialize(    
      test_,    
      CTE_ALL,    
      srv_type_by_index(    
        static_cast<size_t>(CampaignManager)),    
      margin_);    
        
    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    
        
  }  
}
