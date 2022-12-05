
#include "SpentBudgetChecker.hpp"
 
namespace AutoTest
{
  SpentBudgetChecker::SpentBudgetChecker(
    BaseUnit* test,
    const NSLookupRequest& request,
    unsigned long ccgid,
    unsigned long ccid,
    const Generics::Time& deadline) :
    test_(test),
    request_(request),
    ccgid_(ccgid),
    ccid_(ccid),
    deadline_(deadline)
  {}


  SpentBudgetChecker::~SpentBudgetChecker()
    noexcept
  {}

  bool
  SpentBudgetChecker::check(
    bool throw_error)
    /*throw(AutoTest::CheckFailed, eh::Exception)*/
  {
    CampaignChecker campaign_checker(test_, ccgid_,
      CampaignChecker::Expected().eval_status(ccid_ == 0? "I": "A"));

    SelectedCreativeChecker selected_creative_checker(
      AdClient::create_user(test_), request_, ccid_);

    try
    {
      return 
        and_checker(
          wait_checker(
            deadline_ != Generics::Time::ZERO
              ? and_checker(
                  throw_checker(TimeLessChecker(deadline_)),
                  campaign_checker)
              : and_checker(campaign_checker)),
          SelectedCreativeChecker(
            AdClient::create_user(test_),
            request_,
            ccid_)).check(throw_error);
      
    }
    catch (const TimeLessChecker::TimeLessCheckFailed&)
    {
      throw;
    }
    catch (const AutoTest::CheckFailed& e)
    {
      if (deadline_ != Generics::Time::ZERO)
      {
        AutoTest::TimeLessChecker(deadline_).check(true);
      }
      throw;
    }
    return false;
  }
}; //namespace AutoTest
