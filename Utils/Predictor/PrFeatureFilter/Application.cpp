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

#include <Generics/AppUtils.hpp>
#include <Generics/BitAlgs.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/CsvReader.hpp>
#include <Commons/Algs.hpp>

//#include <DTree/TreeLearner.hpp>

#include "Application.hpp"

//using namespace AdServer::CampaignSvcs;

//#define TRACE_OUTPUT

const double EPS = 0.00001;
const double DEPTH_PINALTY = 0.0001;

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "PrFeatureFilter\n";
}

// Application
Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::generate_svm_(
  std::ostream& /*out*/,
  std::istream& /*in*/,
  const char* /*feature_columns_str*/,
  const char* /*result_dictionary*/)
{
}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::StringOption opt_feature_dictionary;
  Generics::AppUtils::StringOption opt_features;
  Generics::AppUtils::StringOption opt_trees_directory;
  Generics::AppUtils::Option<unsigned long> opt_max_features(10);
  Generics::AppUtils::Option<unsigned long> opt_max_top_element(10);
  Generics::AppUtils::Option<unsigned long> opt_depth(5);
  Generics::AppUtils::Option<unsigned long> opt_min_cover(0);
  Generics::AppUtils::StringOption opt_dump_tree;
  Generics::AppUtils::CheckOption opt_xml;
  Generics::AppUtils::StringOption opt_dictionary;
  Generics::AppUtils::StringOption opt_column_names;

  // RAM ~ O(opt_max_top_element ^ opt_depth)
  // RAM ~ 100000 by default
  //
  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);
  args.add(
    Generics::AppUtils::equal_name("max-features") ||
    Generics::AppUtils::short_name("mf"),
    opt_max_features);
  args.add(
    Generics::AppUtils::equal_name("max-top-element") ||
    Generics::AppUtils::short_name("me"),
    opt_max_top_element);
  args.add(
    Generics::AppUtils::equal_name("depth") ||
    Generics::AppUtils::short_name("d"),
    opt_depth);
  args.add(
    Generics::AppUtils::equal_name("dict"),
    opt_feature_dictionary);
  args.add(
    Generics::AppUtils::equal_name("features") ||
    Generics::AppUtils::short_name("f"),
    opt_features);
  args.add(
    Generics::AppUtils::equal_name("trees-directory") ||
    Generics::AppUtils::equal_name("trees-dir") ||
    Generics::AppUtils::short_name("td"),
    opt_trees_directory);
  args.add(
    Generics::AppUtils::equal_name("min-cover") ||
    Generics::AppUtils::short_name("mc"),
    opt_min_cover);
  args.add(
    Generics::AppUtils::equal_name("dump-tree"),
    opt_dump_tree);
  args.add(
    Generics::AppUtils::equal_name("xml"),
    opt_xml);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  std::string command = *commands.begin();

  if(command == "filter-svm")
  {
    std::unordered_set<unsigned long> filter_features;

    {
      // parse filter features
      String::StringManip::Splitter<
        String::AsciiStringManip::SepComma> tokenizer(*opt_features);

      String::SubString token;
      while(tokenizer.get_token(token))
      {
        unsigned long feature_id;
        if(!String::StringManip::str_to_int(token, feature_id))
        {
          Stream::Error ostr;
          ostr << "can't parse label '" << token << "'";
          throw Exception(ostr);
        }

        filter_features.insert(feature_id);
      }
    }

    while(!std::cin.eof())
    {
      BoolLabel label;
      Row_var row = SVM<BoolLabel>::load_line(std::cin, label);

      if(row)
      {
        FeatureArray filtered_features;

        if(!filter_features.empty())
        {
          for(auto it = row->features.begin(); it != row->features.end(); ++it)
          {
            if(filter_features.find(it->first) != filter_features.end())
            {
              filtered_features.push_back(*it);
            }
          }

          row->features.swap(filtered_features);
        }

        // print row
        SVM<BoolLabel>::save_line(std::cout, row, label);
      }
    }
  }
  else if(command == "generate-svm")
  {
    // convert csv to svm
    generate_svm_(
      std::cout,
      std::cin,
      opt_column_names->c_str(),
      opt_dictionary->c_str()
      );
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


