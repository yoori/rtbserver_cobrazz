
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include "CreativeChecker.hpp"

namespace AutoTest
{
  bool  
  CreativeChecker::check(bool throw_error)  
    /*throw(CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<CreativeAdmin, CT_ALL> admins;    
        
    admins.initialize(    
      test_,    
      CTE_ALL,    
      srv_type_by_index(    
        static_cast<size_t>(CampaignManager)),    
      ccid_);    
        
    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    
        
  }  
}
