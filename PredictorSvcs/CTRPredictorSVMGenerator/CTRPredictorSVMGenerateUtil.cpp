#include <list>
#include <iterator>
#include <iostream>
#include <sstream>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/null.hpp>

#include <String/StringManip.hpp>
#include <Generics/AppUtils.hpp>

#include "Utils.hpp"                                                                                                                     
#include "OutputLibSvmStream.hpp"                                                                                                        
#include "InputCsvStream.hpp"

namespace
{
  const char USAGE[] =
    "[OPTIONS] [COMMAND]\n"
    "COMMANDS:\n"
    "  help\n"
    "  csv-to-svm\n"
    "OPTIONS:\n";
}

namespace AdServer::Predictor
{
  void generate_svm(
    std::ostream& out,
    std::istream& in,
    unsigned long feature_dimension)
  {
    const unsigned long index_shifter = sizeof(uint32_t) * 8 - feature_dimension;
    boost::iostreams::stream<boost::iostreams::null_sink> null_dict(
      boost::iostreams::null_sink());
    std::ofstream o("/dev/null", std::ofstream::out | std::ofstream::app );
    OutputLibSvm_var output_svm(new OutputLibSvmStream(
      o,
      out,
      index_shifter,
      nullptr));
    AdServer::Predictor::InputCsvStream::process(*output_svm, in, nullptr, nullptr);
  }
}

int main(int argc, char** argv)
{
  Generics::AppUtils::Option<unsigned long> opt_features_dimension(24);
  Generics::AppUtils::Args args(-1);
  args.add(
    Generics::AppUtils::equal_name("features-dimension") ||
    Generics::AppUtils::short_name("d"),
    opt_features_dimension);
  args.parse(argc - 1, argv + 1);
  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  try
  {
    std::string command = *commands.begin();
    if (command == "csv-to-svm")
    {
      AdServer::Predictor::generate_svm(
        std::cout, std::cin, *opt_features_dimension);
    }
    else
    {
      return 1;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
