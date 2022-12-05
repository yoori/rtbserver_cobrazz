
#include "GlobalParamsGranularUpdateTest.hpp"
#include <Generics/Time.hpp>

REFLECT_UNIT(GlobalParamsGranularUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW,
  AUTO_TEST_SERIALIZE
);

namespace
{
  const unsigned long TWO_DAYS = 2*60*60*24;
  typedef AutoTest::GlobalsChecker GlobalsChecker;
}

// Exchange wrapper implementation

GlobalParamsGranularUpdateTest::Exchange::Exchange(
  DB::IConn& connection,
  unsigned int id)
  : connection_(connection),
    currency_exchange_(connection_)
{
  currency_exchange_.select(id);
}

GlobalParamsGranularUpdateTest::Exchange::Exchange(
  DB::IConn& connection,
  const AutoTest::Time& effective_date)
  : connection_(connection),
    currency_exchange_(connection_)
{
  currency_exchange_.effective_date = effective_date;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      currency_exchange_.insert()),
    "Cann't insert currency exchange");

  add_currencies_();
}

GlobalParamsGranularUpdateTest::Exchange::~Exchange()
{}

const ORM::ORMInt::value_type& GlobalParamsGranularUpdateTest::Exchange::id()
{
  return currency_exchange_.id();
}

bool GlobalParamsGranularUpdateTest::Exchange::del()
{
  delete_currencies_();
  return currency_exchange_.delet();
}

void GlobalParamsGranularUpdateTest::Exchange::add_currencies_()
{
  DB::Query query(connection_.get_query(
    "INSERT INTO CURRENCYEXCHANGERATE "
      "(CURRENCY_EXCHANGE_ID, CURRENCY_ID, RATE) "
    "SELECT :i1, CURRENCY_ID, RATE FROM CURRENCYEXCHANGERATE "
    "WHERE CURRENCY_EXCHANGE_ID = (SELECT MAX(CURRENCY_EXCHANGE_ID) "
      "FROM CURRENCYEXCHANGERATE)"));
  ORM::ORMValue <int> id(currency_exchange_.id());
  query  <<  id;
  query.update();    
  connection_.commit();    
}

void GlobalParamsGranularUpdateTest::Exchange::delete_currencies_()
{
  DB::Query query(connection_.get_query(
    "DELETE FROM CURRENCYEXCHANGERATE WHERE CURRENCY_EXCHANGE_ID = :i1"));
  ORM::ORMValue <int> id(currency_exchange_.id());
  query  <<  id;
  query.update();
  connection_.commit();
}

// GlobalParamsGranularUpdateTest implementation

void 
GlobalParamsGranularUpdateTest::set_up()
{
  add_descr_phrase("SetUp");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_MANAGER)),
    "CampaignManager must set in the XML configuration file");
  frontend_ = get_config().get_service(CTE_ALL, STE_CAMPAIGN_MANAGER).address;

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      GlobalsChecker(
        this,
        GlobalsChecker::Expected().
          currency_exchange_id(
            fetch_int("CurrencyExchange")))).check(),
    "Globals initial");
}

void 
GlobalParamsGranularUpdateTest::tear_down()
{
  add_descr_phrase("Tear down");
}

bool
GlobalParamsGranularUpdateTest::run()
{
  check_currency_exchange_();
  restore_default_values_();
  return true;
}

void GlobalParamsGranularUpdateTest::check_currency_exchange_()
{
  add_descr_phrase("Global currency exchange");
  AutoTest::Time afrertommorow(AutoTest::Time() + TWO_DAYS);

  GlobalParamsGranularUpdateTest::Exchange
    exchange(pq_conn_, afrertommorow);

  currency_exchange_id_ = exchange.id();

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      GlobalsChecker(
        this,
        GlobalsChecker::Expected().
          currency_exchange_id(
            exchange.id()))).check(),
    "Currency exchange changed");

}

void GlobalParamsGranularUpdateTest::restore_default_values_() 
{
  // Restore AdsConfig defaults
  try
  {
    if (currency_exchange_id_ != fetch_int("CurrencyExchange"))
    {
      GlobalParamsGranularUpdateTest::Exchange exchange(
        pq_conn_, currency_exchange_id_);
      exchange.del();
      currency_exchange_id_ = fetch_int("CurrencyExchange");
    }
  }
  catch (...)
  {
    // suppress any exception
  }

  // Check CampaignAdmin defaults after its restore.

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      GlobalsChecker(
        this,
        GlobalsChecker::Expected().
          currency_exchange_id(
            fetch_int("CurrencyExchange")))).check(),
    "Restore currency exchange");
}
