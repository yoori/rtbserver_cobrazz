
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch>  
  bool  
  FreqCapProfileChecker_<ExpectedType, ch>::check(bool throw_error)  
    /*throw (CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<FreqCapProfileAdmin, ch> admins;    

    admins.initialize(    
      test_,    
      CTE_ALL,    
      srv_type_by_index(    
        static_cast<size_t>(service_)),    
      uid_,    
      temp_,    
      service_    
    );    

    admins.log(Logger::thlog());

    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    

  }  
}
