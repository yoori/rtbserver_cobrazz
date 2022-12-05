#include <list>
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>

#include <Generics/AppUtils.hpp>
#include <Generics/BitAlgs.hpp>
#include <xsd/Utils/SVMGeneratorConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/CsvReader.hpp>
#include <ProfilingCommons/PlainStorage3/FileWriter.hpp>
#include <LogCommons/CsvUtils.hpp>

#include "HashCalculator.hpp"
#include "Application.hpp"

using namespace AdServer::CampaignSvcs;

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "SVMGenerator generate-svm <CONFIG DATA FILE> <FEATURE COLUMNS>\n"
    "\n";

  typedef const String::AsciiStringManip::Char2Category<',', '|'>
    ListSepType;
}

// Application
Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::StringOption opt_dictionary;
  Generics::AppUtils::StringOption opt_name_dictionary;
  Generics::AppUtils::StringOption opt_cc_to_ccg_dictionary;
  Generics::AppUtils::StringOption opt_cc_to_campaign_dictionary;
  Generics::AppUtils::StringOption opt_tag_to_publisher_dictionary;
  Generics::AppUtils::CheckOption opt_out_hashes;
  Generics::AppUtils::CheckOption opt_no_check_features;
  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);

  args.add(
    Generics::AppUtils::equal_name("dictionary") ||
    Generics::AppUtils::equal_name("dict") ||
    Generics::AppUtils::short_name("d"),
    opt_dictionary);

  args.add(
    Generics::AppUtils::equal_name("hashes"),
    opt_out_hashes);

  args.add(
    Generics::AppUtils::equal_name("no-check-features"),
    opt_no_check_features);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  std::string command = *commands.begin();

  if(command == "generate-svm")
  {
    Generics::AppUtils::Args::CommandList::const_iterator cmd_it =
      ++commands.begin();

    if(cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-svm: config data file not defined";
      throw Exception(ostr);
    }

    const std::string config_data_file = *cmd_it;

    if(++cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-svm: feature columns not defined";
      throw Exception(ostr);
    }

    const std::string feature_columns = *cmd_it;

    generate_svm_(
      std::cout,
      std::cin,
      config_data_file.c_str(),
      feature_columns.c_str(),
      opt_dictionary->c_str(),
      opt_no_check_features.enabled());
  }
  else
  {
    Stream::Error ostr;
    ostr << "unknown command '" << command << "', "
      "see help for more info" << std::endl;
    throw Exception(ostr);
  }
}

void
Application_::load_dictionary_(
  std::map<std::string, std::string>& dict,
  const char* file)
{
  std::ifstream in(file);
  if(!in.is_open())
  {
    Stream::Error ostr;
    ostr << "can't open '" << file << "'";
    throw Exception(ostr);
  }

  std::vector<std::string> values;

  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    values.resize(0);
    Commons::CsvReader::parse_line(values, line);

    if(values.size() != 2)
    {
      Stream::Error ostr;
      ostr << "invalid dictionary line '" << line << "'";
      throw Exception(ostr);
    }

    dict[values[0]] = values[1];
  }
}

struct HashCalculatorHolder
{
  std::size_t seed;
  HashCalculator_var hash_calculator;
};

void
Application_::generate_svm_(
  std::ostream& out,
  std::istream& in,
  const char* config_file,
  const char* feature_columns_str,
  const char* dictionary_file_path,
  bool no_check_features)
{
  using namespace xsd::AdServer;

  Config::ErrorHandler error_handler;
  std::unique_ptr<Configuration::SVMGeneratorType> config;

  try
  {
    config = Configuration::SVMGenerator(config_file, error_handler);
  }
  catch(const xml_schema::parsing& e)
  {
    Stream::Error ostr;
    ostr << "Can't parse config file '" << config_file << "': ";
    if(error_handler.has_errors())
    {
      std::string error_string;
      ostr << error_handler.text(error_string);
    }

    throw Exception(ostr);
  }

  // parse model config
  unsigned long dimension = config->Model().features_dimension();
  unsigned long index_shifter = sizeof(uint32_t)*8 - dimension;

  // parse columns (name => {index, soil})
  std::map<std::string, unsigned long> feature_columns;
  std::set<unsigned long> key_columns;

  {
    std::vector<std::string> feature_column_names;
    Commons::CsvReader::parse_line(
      feature_column_names,
      String::SubString(feature_columns_str));

    unsigned long column_i = 0;
    for(auto it = feature_column_names.begin(); it != feature_column_names.end(); ++it, ++column_i)
    {
      String::AsciiStringManip::to_lower(*it);

      if(!it->empty() && (*it)[0] == '+')
      {
        key_columns.insert(column_i);
        *it = std::string(*it, 1);
      }

      feature_columns.insert(std::make_pair(*it, column_i));
      //std::cerr << "COLUMN " << *it << ": " << column_i << std::endl;
    }
  }

  // configure model
  std::vector<HashCalculatorHolder> hash_calculators;

  for(Configuration::ModelType::Feature_sequence::const_iterator feature_it =
        config->Model().Feature().begin();
      feature_it != config->Model().Feature().end(); ++feature_it)
  {
    // eval seed
    std::string hc_name;
    HashCalculatorHolder hash_calculator_holder;

    {
      Generics::Murmur32v3Hash hash(hash_calculator_holder.seed);
      for(Configuration::FeatureType::BasicFeature_sequence::
            const_iterator basic_feature_it =
              feature_it->BasicFeature().begin();
          basic_feature_it != feature_it->BasicFeature().end();
          ++basic_feature_it)
      {
        std::string name = basic_feature_it->name();
        String::AsciiStringManip::to_lower(name);
        hash.add(name.data(), name.size());

        if(basic_feature_it != feature_it->BasicFeature().begin())
        {
          hc_name += '/';
        }

        hc_name += name;
      }
    }

    for(Configuration::FeatureType::BasicFeature_sequence::
          const_iterator basic_feature_it =
            feature_it->BasicFeature().begin();
        basic_feature_it != feature_it->BasicFeature().end();
        ++basic_feature_it)
    {
      auto column_it = feature_columns.find(basic_feature_it->name());
      unsigned long feature_col = 0xFFFFFFFF;

      if(!no_check_features && column_it == feature_columns.end())
      {
        Stream::Error ostr;
        ostr << "Invalid basic feature name: '" << basic_feature_it->name() << "'";
        throw Exception(ostr);
      }
      else if(column_it != feature_columns.end())
      {
        feature_col = column_it->second;
      }

      HashCalculator_var prev_hash_calculator = hash_calculator_holder.hash_calculator;
      if(prev_hash_calculator)
      {
        hash_calculator_holder.hash_calculator = new HashCalculatorDelegateImpl(
          index_shifter,
          prev_hash_calculator,
          feature_col);
      }
      else
      {
        if(feature_it->is_float())
        {
          hash_calculator_holder.hash_calculator = new HashCalculatorFloatFinalImpl(
            index_shifter,
            hc_name.c_str(),
            feature_col);
        }
        else
        {
          hash_calculator_holder.hash_calculator = new HashCalculatorFinalImpl(
            index_shifter,
            hc_name.c_str(),
            feature_col);
        }
      }
    }

    hash_calculators.push_back(hash_calculator_holder);
  }

  std::unique_ptr<HashDictionary> dict_table;
  if(dictionary_file_path[0])
  {
    dict_table.reset(new HashDictionary());
  }

  // fetch input (values)
  unsigned long line_i = 0;
  HashCalculateParams params;
  params.values.reserve(feature_columns.size());

  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    params.values.clear();

    bool skip_line = false;

    std::vector<std::string> column_values;

    try
    {
      Commons::CsvReader::parse_line(column_values, line);

      for(auto column_value_it = column_values.begin();
        column_value_it != column_values.end();
        ++column_value_it)
      {
        params.values.push_back(HashCalculateParams::Value());

        String::StringManip::Splitter<
          String::AsciiStringManip::SepBar> tokenizer(*column_value_it);
        String::SubString token;
        while(tokenizer.get_token(token))
        {
          params.values.back().push_back(token.str());
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Line #" << line_i << ": " << ex.what() << ex.what();
      std::cerr << ostr.str() << std::endl;

      skip_line = true;
    }

    if(skip_line)
    {
      continue;
    }

    // eval hashes
    HashArray hashes;
    unsigned long hash_i = 0;

    for(auto hash_calc_it = hash_calculators.begin();
      hash_calc_it != hash_calculators.end();
      ++hash_calc_it, ++hash_i)
    {
      try
      {
        Murmur32v3Adapter hash_adapter(hash_calc_it->seed);
        hash_calc_it->hash_calculator->eval_hashes(
          hashes,
          hash_adapter,
          params);
      }
      catch(const eh::Exception& ex)
      {
        std::cerr << "For hash_calculator #" << hash_i << ": " << ex.what() << std::endl;
        assert(0);
      }
    }

    // output hashes
    std::ostringstream res_line_ostr;

    for(auto key_it = key_columns.begin(); key_it != key_columns.end(); ++key_it)
    {
      res_line_ostr << column_values[*key_it] << ',';
    }

    std::map<unsigned long, double> ordered_hashes;

    for(auto hash_it = hashes.begin();
      hash_it != hashes.end(); ++hash_it)
    {
      ordered_hashes.insert(*hash_it);
    }

    for(auto hash_it = ordered_hashes.begin();
      hash_it != ordered_hashes.end(); ++hash_it)
    {
      if(hash_it != ordered_hashes.begin())
      {
        res_line_ostr << ' ';
      }
      res_line_ostr << hash_it->first << ":";
      if(std::abs(hash_it->second - 1.0) > 0.0000001)
      {
        res_line_ostr << std::fixed << std::setprecision(8);
      }
      res_line_ostr << hash_it->second;
    }

    if(dict_table.get())
    {
      // fill dict
      std::vector<String::SubString> value_path;
      for(auto hash_calc_it = hash_calculators.begin();
        hash_calc_it != hash_calculators.end(); ++hash_calc_it)
      {
        Murmur32v3Adapter hash_adapter(hash_calc_it->seed);
        hash_calc_it->hash_calculator->fill_dictionary(
          *dict_table,
          hash_adapter,
          params,
          value_path);
      }
    }

    // push line to output
    out << res_line_ostr.str() << std::endl;
    ++line_i;
  }

  // dump dictionaries
  if(dict_table.get())
  {
    std::ofstream dictionary_file(dictionary_file_path, std::ios_base::out);
    for(auto it = dict_table->begin(); it != dict_table->end(); ++it)
    {
      dictionary_file << it->first << ",";
      if(!it->second.empty())
      {
        std::string res = it->second;

        AdServer::LogProcessing::write_not_empty_string_as_csv(
          dictionary_file,
          res);
      }
      dictionary_file << std::endl;
    }
    dictionary_file.close();
  }
}

int main(int argc, char** argv)
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


