
#ifndef __AUTOTESTS_COMMONS_ADMINS_CAMPAIGNCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CAMPAIGNCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/CampaignAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  /*
   * @class CampaignChecker
   * @brief CampaignAdmin call checking wrapper
   */
  class CampaignChecker:  
    public AutoTest::Checker  
  {  
  public:

    enum NotPresentTag
    {
      NOT_PRESENT
    };

    typedef CampaignAdmin::Expected Expected;      
    
    /**
     * @brief Constructor.
     *
     * @param reference to global AutoTests params.
     * @param CCG id.
     * @param expected values set, if not defined simple check that campaign exists
     */
    CampaignChecker(
      BaseUnit* test,
      unsigned long ccg_id,
      const Expected& expected = Expected(),
      CampaignAdmin::Modificator expand = CampaignAdmin::EXPAND);

    CampaignChecker(
      BaseUnit* test,
      unsigned long ccg_id,
      NotPresentTag);
            
    virtual ~CampaignChecker() noexcept;
            
    bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
  private:    

    BaseUnit* test_;      
    unsigned long ccg_id_;      
    Expected expected_;      
    CampaignAdmin::Modificator expand_channels_;
    AdminExistCheck exists_;
    static const Expected ANY_EXPECTED_;
  };  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_CAMPAIGNCHECKER_HPP

