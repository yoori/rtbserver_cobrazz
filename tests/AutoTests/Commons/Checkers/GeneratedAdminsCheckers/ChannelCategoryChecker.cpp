
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include "ChannelCategoryChecker.hpp"

namespace AutoTest
{
  bool
  ChannelCategoryChecker::check(bool throw_error)
    /*throw(CheckFailed, eh::Exception)*/
  {  
    AdminsArray<ChannelCategoryAdmin, CT_ALL> admins;
  
    admins.initialize(
      test_,
      CTE_ALL,
      srv_type_by_index(
        static_cast<size_t>(CampaignManager)),
      category_);
    
    return admin_checker(
      admins,
      expected_,
      exists_).check(throw_error);
  }  
}
