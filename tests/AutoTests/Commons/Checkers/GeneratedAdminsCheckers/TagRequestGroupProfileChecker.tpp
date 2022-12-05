namespace AutoTest
{
  template<typename ExpectedType, CheckType ch>
  bool  
  TagRequestGroupProfileChecker_<ExpectedType, ch>::check(bool throw_error)  
    /*throw (CheckFailed, eh::Exception)*/  
  {  
    AdminsArray<TagRequestGroupProfileAdmin, ch> admins;    
        
    admins.initialize(    
      test_,    
      CTE_ALL,    
      srv_type_by_index(    
        static_cast<size_t>(service_)),    
      uuid_,    
      service_    
    );    
        
    return admin_checker(    
      admins,    
      expected_,    
      exists_).check(throw_error);    
        
  }  
}
