namespace AutoTest
{
  template<typename ExpectedType, CheckType ch>
  bool  
  UserTriggerMatchProfileChecker_<ExpectedType, ch>::check(bool throw_error)  
    /*throw (CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<UserTriggerMatchProfileAdmin, ch> admins;    
        
    admins.initialize(    
      test_,    
      CTE_ALL,    
      srv_type_by_index(    
        static_cast<size_t>(ExpressionMatcher)),    
      uid_,    
      temp_    
    );    
        
    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    
        
  }  
}
