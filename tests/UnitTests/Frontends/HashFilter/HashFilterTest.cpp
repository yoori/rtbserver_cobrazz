#include <iostream>
#include <eh/Exception.hpp>

#include <Frontends/ProfilingServer/HashFilter.hpp>

namespace UnitTests
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
}

int
main(/*int argc, char* argv[]*/)
{
  AdServer::Profiling::HashFilter_var hash_filter = new AdServer::Profiling::HashFilter(
    100,
    1000,
    Generics::Time(10),
    Generics::Time::ONE_MINUTE);

  Generics::Time now = Generics::Time::get_time_of_day();

  // check now filtering
  std::cout << "NOW: " << now.gm_ft() << std::endl;

  bool res = hash_filter->set(10000, now, now);
  std::cout << "set #1.1(, " << now.gm_ft() << "): " << res << std::endl;

  res = hash_filter->set(10000, now, now);
  std::cout << "set #1.2(, " << now.gm_ft() << "): " << res << std::endl;

  // check already expired (both false)
  res = hash_filter->set(10000, now - Generics::Time::ONE_MINUTE, now);
  std::cout << "set #2.1(, " << now.gm_ft() << "): " << res << std::endl;

  res = hash_filter->set(10000, now - Generics::Time::ONE_MINUTE, now);
  std::cout << "set #2.2(, " << now.gm_ft() << "): " << res << std::endl;

  // check last second before expiration
  res = hash_filter->set(10000, now - Generics::Time::ONE_MINUTE + Generics::Time::ONE_SECOND * 11, now);
  std::cout << "set #3.1(, " << now.gm_ft() << "): " << res << std::endl;

  res = hash_filter->set(10000, now - Generics::Time::ONE_MINUTE + Generics::Time::ONE_SECOND * 11, now);
  std::cout << "set #3.2(, " << now.gm_ft() << "): " << res << std::endl;


  res = hash_filter->set(10000, now - Generics::Time(10), now);
  std::cout << "set #4.1(, " << now.gm_ft() << "): " << res << std::endl;

  res = hash_filter->set(10000, now - Generics::Time(10), now);
  std::cout << "set #4.2(, " << now.gm_ft() << "): " << res << std::endl;


  res = hash_filter->set(10000, now - Generics::Time(10), now + Generics::Time::ONE_SECOND * 30);
  std::cout << "set #5(, " << (now + Generics::Time::ONE_SECOND * 30).gm_ft() << "): " << res << std::endl;


  res = hash_filter->set(10000000, now - Generics::Time(10), now + Generics::Time::ONE_SECOND * 30);
  std::cout << "set #6.1(, " << (now + Generics::Time::ONE_SECOND * 30).gm_ft() << "): " << res << std::endl;

  res = hash_filter->set(10000000, now - Generics::Time(10), now + Generics::Time::ONE_SECOND * 30);
  std::cout << "set #6.2(, " << (now + Generics::Time::ONE_SECOND * 30).gm_ft() << "): " << res << std::endl;

  return 0;
}

