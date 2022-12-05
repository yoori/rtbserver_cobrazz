
#include <iostream>
#include <sstream>

#include "Application.hpp"

namespace
{
  const char usage[] = "usage: AdServerTest <log_level> <test_configfile_path>";
}


int main (int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "error: test configuration file path not specified" << std::endl <<
      usage << std::endl;
    exit(1);
  }
  try
    {
      //ACE::init ();
      Application app(atoi(argv[1]), argv[2]);
      app.init();
      app.run();
      //ACE::fini ();
    }
  catch (eh::Exception& e)
    {
      std::cerr << "Exception:" << e.what() << std::endl;
      exit(1);      
    }
  catch (...)
    {
      std::cerr << "Unexpected exception" << std::endl;
      exit(1);      
    }
}
