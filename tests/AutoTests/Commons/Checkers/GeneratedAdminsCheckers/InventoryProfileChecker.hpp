
#ifndef __AUTOTESTS_COMMONS_ADMINS_INVENTORYPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_INVENTORYPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/InventoryProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch = CT_ONE>
  class InventoryProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      InventoryProfileChecker_(      
        BaseUnit* test,      
        const std::string& uid,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        uid_(uid),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~InventoryProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string uid_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  

  typedef InventoryProfileChecker_<InventoryProfileAdmin::Expected> InventoryProfileChecker;
  typedef InventoryProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> InventoryProfileEmptyChecker;

}

#include "InventoryProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_INVENTORYPROFILECHECKER_HPP

