#include <list>
#include <vector>
#include <iterator>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <unordered_set>

#include <Generics/Rand.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/BitAlgs.hpp>
#include <Generics/TaskRunner.hpp>
#include <GeoIP/IPMap.hpp>
#include <String/AsciiStringManip.hpp>

#include <LogCommons/CsvUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/CsvReader.hpp>
#include <Commons/Algs.hpp>
#include <Commons/FileManip.hpp>

#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "GeoUtil add-geo\n";
}

// Application
Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::add_geo_(
  std::istream& in,
  std::ostream& out)
{
  std::unique_ptr<GeoIPMapping::IPMapCity> ip_map(new GeoIPMapping::IPMapCity(0));

  while(!in.eof())
  {
    std::string line;
    std::getline(in, line);
    if(!line.empty())
    {
      std::string loc = "//";

      try
      {
        GeoIPMapping::IPMapCity::CityLocation geo_location;
        ip_map->city_location_by_addr(line.c_str(), geo_location, false);
        loc = geo_location.country_code + "/" +
          geo_location.region + "/" +
          geo_location.city;
        String::AsciiStringManip::to_lower(loc);
      }
      catch(const eh::Exception&)
      {}

      out << line << ",";
      AdServer::LogProcessing::write_not_empty_string_as_csv(out, loc);
      out << std::endl;
    }
  }
}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;

  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  auto command_it = commands.begin();
  std::string command = *command_it;
  ++command_it;

  if(command == "add-geo")
  {
    add_geo_(std::cin, std::cout);
  }
  else
  {
    Stream::Error ostr;
    ostr << "unknown command '" << command << "', "
      "see help for more info" << std::endl;
    throw Exception(ostr);
  }
}

// main
int
main(int argc, char** argv)
{
  Application_* app = 0;

  try
  {
    app = &Application::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  assert(app);

  try
  {
    app->main(argc, argv);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}


