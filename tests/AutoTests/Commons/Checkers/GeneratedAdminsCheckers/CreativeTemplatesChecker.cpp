
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include "CreativeTemplatesChecker.hpp"

namespace AutoTest
{
  bool  
  CreativeTemplatesChecker::check(bool throw_error)  
    /*throw(CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<CreativeTemplatesAdmin, CT_ALL> admins;    
        
    admins.initialize(    
      test_,    
      CTE_ALL,    
      srv_type_by_index(    
        static_cast<size_t>(CampaignManager))    
    );    
        
    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    
        
  }  
}
