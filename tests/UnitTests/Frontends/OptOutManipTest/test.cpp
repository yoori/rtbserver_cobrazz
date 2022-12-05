#include<iostream>
#include<Generics/Time.hpp>
#include <Frontends/FrontendCommons/OptOutManip.hpp> 


int main(int argc, char* argv[])
{
  if(argc>2)
  {
    std::cout << "usage OptOutTest [days]" << std::endl;
    return 1;
  }
  unsigned long days =0;
  if(argc==2)
  {
    std::stringstream convert;
    convert << argv[1];
    convert >> days;
  }
  std::cout << "days: " << days << std::endl;
  unsigned long tm = time(0);
  std::cout << "start time: " << tm << std::endl;
  tm += (days * 24 * 3600);
  std::cout << "time with ofset: " << tm << std::endl;
  Generics::Time opt_in_days_time(tm), restore_time;
  std::cout << "Time: " << opt_in_days_time << std::endl;
  std::string buf;
  AdServer::OptInDays::save_opt_in_days(opt_in_days_time, buf);
  AdServer::OptInDays::load_opt_in_days(buf.c_str(), restore_time);
  std::cout << "Restore time: " << restore_time << std::endl;
return 0;
}

