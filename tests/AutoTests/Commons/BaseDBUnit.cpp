
#include <tests/AutoTests/Commons/Common.hpp>

bool BaseDBUnit::run_test()
{
  bool ret = false;
  try
  {
    set_up();
    if (stat_.db_active())
    {
      pre_condition();
      ret = run ();
      post_condition();
    }
    else
    {
      ret = run();
    }
    add_descr_phrase("Changes check.");
    check();
  }
  catch(const eh::Exception& exc)
  {
    AutoTest::Logger::thlog().log(String::SubString(exc.what()), Logging::Logger::ERROR);
    safe_tear_down ();
    throw;
  }
  catch(...)
  {
    AutoTest::Logger::thlog().log(String::SubString("Unknown exception"),
      Logging::Logger::ERROR);
    safe_tear_down ();
    throw;
  }
  restorers_.clear();  
  tear_down ();
  return ret;
}

void BaseDBUnit::safe_tear_down()
{
  try
  {
    restorers_.clear();
    tear_down();
  }
  catch (const eh::Exception& e)
  {
    AutoTest::Logger::thlog().log(e.what(), Logging::Logger::ERROR); 
    stat_.error += e.what();
  }
  catch(...)
  {
    AutoTest::Logger::thlog().log("Unknown exception",
      Logging::Logger::ERROR);
  }
  
}

BaseDBUnit::BaseDBUnit(UnitStat& stat_var, 
                       const char* task_name, 
                       XsdParams params_var) :
  BaseUnit(stat_var, task_name, params_var),
  pq_conn_(open_pq())
{ }

BaseDBUnit::~BaseDBUnit() noexcept
{
  try 
  {
    restorers_.clear();
    pq_conn_.close();
  }
  catch(const eh::Exception& exc)
  {
    AutoTest::Logger::thlog().log(exc.what(), Logging::Logger::ERROR);
  }
  catch (...)
  {
    AutoTest::Logger::thlog().log("Unknown exception",
      Logging::Logger::ERROR);
  }
}

void BaseDBUnit::set_up()
{ }

void BaseDBUnit::pre_condition()
{ }

void BaseDBUnit::post_condition()
{ }
