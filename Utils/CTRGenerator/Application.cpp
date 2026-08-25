#include <list>
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>
#include <cstdint>

#include <arpa/inet.h>

#include <Generics/AppUtils.hpp>
#include <Generics/BitAlgs.hpp>
#include <xsd/Utils/CTRGeneratorDataConfig.hpp>
#include <Generics/MonoAllocator.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/CsvReader.hpp>
#include <ProfilingCommons/FileWriter.hpp>
#include <LogCommons/CsvUtils.hpp>
#include <CampaignSvcs/CampaignManager/CTR/XGBoostPredictor.hpp>
#include <CampaignSvcs/CampaignManager/CTR/FeatureHash.hpp>
#include <CampaignSvcs/CampaignManager/CTRProviderImpl.hpp>

#include "CTRGenerator.hpp"
#include "CalculateParamsFilter.hpp"
#include "EntityUtils.hpp"
#include "FeatureConfig.hpp"
#include "Application.hpp"

using namespace AdServer::CampaignSvcs;

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "CTRGenerator generate-model <FEATURE CONFIG JSON> <CONFIG DATA FILE> "
      "<RESULT WEIGHT FILE>\n"
    "CTRGenerator generate-svm <FEATURE CONFIG JSON> <FEATURE COLUMNS> "
      "[--model=xgboost|catboost] [--feature-indexes-file=<FILE>] "
      "[--feature-stats=<FILE>]\n"
    "\n"
    "CTRGenerator generate-xgb-ctr <XGB MODEL FILE> <FEATURE CONFIG JSON> "
      "<FEATURE COLUMNS>\n"
    "CTRGenerator generate-ctr <CONFIG DIR> <FEATURE COLUMNS> [LINE]\n";
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
  Generics::AppUtils::StringOption opt_model("xgboost");
  Generics::AppUtils::StringOption opt_feature_indexes_file;
  Generics::AppUtils::StringOption opt_feature_stats_file;
  Generics::AppUtils::CheckOption opt_out_hashes;
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
    Generics::AppUtils::equal_name("name-dictionary") ||
    Generics::AppUtils::equal_name("name-dict") ||
    Generics::AppUtils::short_name("nd"),
    opt_name_dictionary);

  args.add(
    Generics::AppUtils::equal_name("cc-to-ccg"),
    opt_cc_to_ccg_dictionary);

  args.add(
    Generics::AppUtils::equal_name("cc-to-campaign"),
    opt_cc_to_campaign_dictionary);

  args.add(
    Generics::AppUtils::equal_name("tag-to-publisher"),
    opt_tag_to_publisher_dictionary);

  args.add(
    Generics::AppUtils::equal_name("hashes"),
    opt_out_hashes);

  args.add(
    Generics::AppUtils::equal_name("model"),
    opt_model);

  args.add(
    Generics::AppUtils::equal_name("feature-indexes-file"),
    opt_feature_indexes_file);

  args.add(
    Generics::AppUtils::equal_name("feature-stats"),
    opt_feature_stats_file);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  std::string command = *commands.begin();

  if(command == "generate-model")
  {
    Generics::AppUtils::Args::CommandList::const_iterator cmd_it =
      ++commands.begin();

    if(cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-model: config file not defined";
      throw Exception(ostr);
    }

    std::string config_file = *cmd_it;

    if(++cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-model: config data file not defined";
      throw Exception(ostr);
    }

    std::string config_data_file = *cmd_it;

    if(++cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-model: result file not defined";
      throw Exception(ostr);
    }

    std::string weight_file = *cmd_it;

    generate_model_(
      std::cout,
      weight_file.c_str(),
      config_file.c_str(),
      config_data_file.c_str());
  }
  else if(command == "generate-svm")
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
    std::string feature_columns;

    if(++cmd_it != args.commands().end())
    {
      feature_columns = *cmd_it;
    }
    else
    {
      std::getline(std::cin, feature_columns);
    }

    bool catboost_model;
    if(*opt_model == "catboost")
    {
      catboost_model = true;
    }
    else if(*opt_model == "xgboost")
    {
      catboost_model = false;
    }
    else
    {
      Stream::Error ostr;
      ostr << "generate-svm: unsupported model '" << *opt_model << "'";
      throw Exception(ostr);
    }

    generate_svm_(
      std::cout,
      std::cin,
      config_data_file.c_str(),
      feature_columns.c_str(),
      opt_cc_to_ccg_dictionary->c_str(),
      opt_cc_to_campaign_dictionary->c_str(),
      opt_tag_to_publisher_dictionary->c_str(),
      opt_dictionary->c_str(),
      opt_name_dictionary->c_str(),
      opt_feature_indexes_file->c_str(),
      opt_feature_stats_file->c_str(),
      catboost_model);
  }
  else if(command == "generate-xgb-ctr")
  {
    Generics::AppUtils::Args::CommandList::const_iterator cmd_it =
      ++commands.begin();

    if(cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-xgb-ctr: xgb model not defined";
      throw Exception(ostr);
    }

    const std::string xgb_model_file = *cmd_it;

    if(++cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-xgb-ctr: config data file not defined";
      throw Exception(ostr);
    }

    const std::string config_data_file = *cmd_it;

    if(++cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-xgb-ctr: feature columns not defined";
      throw Exception(ostr);
    }

    const std::string feature_columns = *cmd_it;

    generate_xgb_ctr_(
      std::cout,
      std::cin,
      xgb_model_file.c_str(),
      config_data_file.c_str(),
      feature_columns.c_str(),
      opt_cc_to_ccg_dictionary->c_str(),
      opt_cc_to_campaign_dictionary->c_str(),
      opt_tag_to_publisher_dictionary->c_str(),
      opt_out_hashes.enabled());
  }
  else if(command == "generate-ctr")
  {
    Generics::AppUtils::Args::CommandList::const_iterator cmd_it =
      ++commands.begin();

    if(cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-ctr: config folder not defined";
      throw Exception(ostr);
    }

    const std::string config_dir = *cmd_it;

    if(++cmd_it == args.commands().end())
    {
      Stream::Error ostr;
      ostr << "generate-ctr: feature columns not defined";
      throw Exception(ostr);
    }

    const std::string feature_columns = *cmd_it;

    if(++cmd_it != args.commands().end())
    {
      std::istringstream istr(*cmd_it);

      generate_ctr_(
        std::cout,
        istr,
        config_dir.c_str(),
        feature_columns.c_str(),
        opt_cc_to_ccg_dictionary->c_str(),
        opt_cc_to_campaign_dictionary->c_str(),
        opt_tag_to_publisher_dictionary->c_str(),
        opt_out_hashes.enabled());
    }
    else
    {
      generate_ctr_(
        std::cout,
        std::cin,
        config_dir.c_str(),
        feature_columns.c_str(),
        opt_cc_to_ccg_dictionary->c_str(),
        opt_cc_to_campaign_dictionary->c_str(),
        opt_tag_to_publisher_dictionary->c_str(),
        opt_out_hashes.enabled());
    }
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
Application_::generate_model_(
  std::ostream& out,
  const char* out_weight_file,
  const char* config_file,
  const char* config_data_file)
{
  using namespace xsd::AdServer;

  const auto config = load_ctr_generator_feature_config(config_file);

  Config::ErrorHandler error_handler;

  std::unique_ptr<Configuration::CTRGeneratorDataType> config_data;

  try
  {
    config_data = Configuration::CTRGeneratorData(config_data_file, error_handler);
  }
  catch(const xml_schema::parsing& e)
  {
    Stream::Error ostr;
    ostr << "Can't parse config data file '" << config_data_file << "': ";
    if(error_handler.has_errors())
    {
      std::string error_string;
      ostr << error_handler.text(error_string);
    }

    throw Exception(ostr);
  }

  // parse model config
  const unsigned long dimension = config.features_dimension;
  const unsigned long features_size = 1UL << dimension;
  CTR::FeatureNameResolver feature_name_resolver;

  // fetch values
  std::vector<float> weights;
  weights.resize(features_size, config_data->default_weight());

  CTRGenerator ctr_generator(config.features, false);
  CalculateParamsFiller calc_params_filler;

  unsigned long value_i = 0;

  weights[0] = config_data->base_weight();

  for(Configuration::CTRGeneratorDataType::Value_sequence::
        const_iterator val_it = config_data->Value().begin();
      val_it != config_data->Value().end();
      ++val_it, ++value_i)
  {
    CTRGenerator::CalculateParams calc_params;

    for(Configuration::ValueType::BasicFeatureValue_sequence::
          const_iterator bf_it = val_it->BasicFeatureValue().begin();
        bf_it != val_it->BasicFeatureValue().end(); ++bf_it)
    {
      CTR::BasicFeature basic_feature;
      if(!feature_name_resolver.basic_feature_by_name(
           basic_feature,
           bf_it->name()))
      {
        Stream::Error ostr;
        ostr << "Invalid basic feature name: '" << bf_it->name() << "'";
        throw Exception(ostr);
      }

      if(basic_feature == CTR::BF_CAMPAIGN_FREQ_LOG_ID)
      {
        Stream::Error ostr;
        ostr << "Feature '" << bf_it->name() << "' can't be defined directly, "
          "use 'campaign_freq' instead";
        throw Exception(ostr);
      }

      calc_params_filler.set_value(calc_params, basic_feature, bf_it->value());
    }

    // process record
    CTRGenerator::Calculation ctr_calculation;
    ctr_generator.calculate(ctr_calculation, calc_params);

    out << "Hashes for record #" << value_i << ":";

    if(!ctr_calculation.hashes.empty())
    {
      unsigned long hashes_count = ctr_calculation.hashes.size();
      float result_weight = val_it->weight();
      float result_weight_part = result_weight / hashes_count;

      for(auto hash_it = ctr_calculation.hashes.begin();
          hash_it != ctr_calculation.hashes.end(); ++hash_it)
      {
        const uint32_t index = CTR::feature_hash_index(
          hash_it->first,
          features_size);
        weights[index] = result_weight_part;

        if(hash_it != ctr_calculation.hashes.begin())
        {
          out << ",";
        }
        out << index << "(" << hash_it->first << ")";
      }
    }

    out << std::endl;
    out << "Weight = " << val_it->weight() << " divided between these hashes" <<
      std::endl;
  }

  // dump weight table
  AdServer::ProfilingCommons::FileWriter weight_file_writer(
    out_weight_file, 10*1024*1024);
  for(std::vector<float>::const_iterator wit = weights.begin();
      wit != weights.end(); ++wit)
  {
    uint32_t val = htonl(*reinterpret_cast<const uint32_t*>(&*wit));
    weight_file_writer.write(&val, sizeof(val));
  }
  weight_file_writer.close();
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

void
Application_::generate_svm_(
  std::ostream& out,
  std::istream& in,
  const char* config_file,
  const char* feature_columns_str,
  const char* cc_to_ccg_dictionary_file_path,
  const char* cc_to_campaign_dictionary_file_path,
  const char* tag_to_publisher_dictionary_file_path,
  const char* dictionary_file_path,
  const char* name_dictionary_file_path,
  const char* feature_indexes_file_path,
  const char* feature_stats_file_path,
  bool catboost_model)
{
  using namespace xsd::AdServer;

  // load creative dictionary
  std::map<std::string, std::string> ccid_to_ccg;
  if(cc_to_ccg_dictionary_file_path[0])
  {
    load_dictionary_(ccid_to_ccg, cc_to_ccg_dictionary_file_path);
  }

  std::map<std::string, std::string> ccid_to_campaign;
  if(cc_to_campaign_dictionary_file_path[0])
  {
    load_dictionary_(ccid_to_campaign, cc_to_campaign_dictionary_file_path);
  }

  // load tag dictionary
  std::map<std::string, std::string> tag_to_publisher;
  if(tag_to_publisher_dictionary_file_path[0])
  {
    load_dictionary_(tag_to_publisher, tag_to_publisher_dictionary_file_path);
  }

  std::map<std::string, std::string> names;
  if(name_dictionary_file_path[0])
  {
    load_dictionary_(names, name_dictionary_file_path);
  }

  const auto config = load_ctr_generator_feature_config(config_file);

  // parse model config
  const unsigned long dimension = config.features_dimension;
  const unsigned long features_size = 1UL << dimension;
  CTR::FeatureNameResolver feature_name_resolver;

  struct FeatureStat
  {
    std::uint64_t impressions = 0;
    std::uint64_t clicks = 0;
  };

  std::vector<FeatureStat> feature_stats;
  std::uint64_t total_impressions = 0;
  std::uint64_t total_clicks = 0;
  if(feature_stats_file_path[0])
  {
    feature_stats.resize(features_size + 1);
  }

  std::vector<unsigned char> allowed_feature_indexes;
  if(feature_indexes_file_path[0])
  {
    allowed_feature_indexes.resize(features_size + 1, 0);
    std::ifstream feature_indexes_file(feature_indexes_file_path);
    if(!feature_indexes_file)
    {
      Stream::Error ostr;
      ostr << "Can't open feature indexes file '" <<
        feature_indexes_file_path << "'";
      throw Exception(ostr);
    }

    unsigned long feature_index;
    unsigned long feature_indexes_count = 0;
    while(feature_indexes_file >> feature_index)
    {
      if(feature_index == 0 || feature_index > features_size)
      {
        Stream::Error ostr;
        ostr << "Invalid feature index " << feature_index << " in '" <<
          feature_indexes_file_path << "', expected range is [1, " <<
          features_size << "]";
        throw Exception(ostr);
      }
      allowed_feature_indexes[feature_index] = 1;
      ++feature_indexes_count;
    }

    if(!feature_indexes_file.eof())
    {
      Stream::Error ostr;
      ostr << "Invalid feature index in '" <<
        feature_indexes_file_path << "'";
      throw Exception(ostr);
    }

    if(feature_indexes_count == 0)
    {
      Stream::Error ostr;
      ostr << "Feature indexes file '" << feature_indexes_file_path <<
        "' is empty";
      throw Exception(ostr);
    }
  }

  // parse columns
  std::map<unsigned long, unsigned long> feature_columns;
  unsigned long label_index = 0;

  {
    std::vector<std::string> feature_column_names;
    Commons::CsvReader::parse_line(
      feature_column_names,
      String::SubString(feature_columns_str));
    unsigned long column_i = 0;
    for(auto it = feature_column_names.begin(); it != feature_column_names.end(); ++it, ++column_i)
    {
      if(*it == "label" || *it == "Label")
      {
        label_index = column_i;
      }
      else if(*it == "timestamp" || *it == "Timestamp")
      {
        feature_columns[CalculateParamsFiller::BF_TIMESTAMP] = column_i;
      }
      else if(*it == "link" || *it == "Link")
      {
        feature_columns[CalculateParamsFiller::BF_LINK] = column_i;
      }
      else if(!it->empty() && (*it)[0] != '#')
      {
        String::AsciiStringManip::to_lower(*it);

        CTR::BasicFeature basic_feature;
        if(!feature_name_resolver.basic_feature_by_name(basic_feature, *it))
        {
          Stream::Error ostr;
          ostr << "Invalid basic feature name: '" << *it << "'";
          throw Exception(ostr);
        }

        feature_columns[basic_feature] = column_i;
      }
    }
  }

  std::unique_ptr<CTRGenerator::FeatureDictionary> dict_table;
  if(dictionary_file_path[0])
  {
    dict_table.reset(new CTRGenerator::FeatureDictionary());
  }

  // fetch input (values)
  CTRGenerator ctr_generator(config.features, !catboost_model);
  CalculateParamsFiller calc_params_filler;

  unsigned long line_i = 0;
  std::vector<std::string> feature_column_values;
  feature_column_values.reserve(feature_columns.size());
  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    feature_column_values.resize(0);

    bool skip_line = false;
    CTRGenerator::CalculateParams calc_params;
    std::string label;

    try
    {
      Commons::CsvReader::parse_line(feature_column_values, line);
      label = feature_column_values[label_index];

      unsigned long column_i = 0;
      for(auto feature_it = feature_columns.begin();
        feature_it != feature_columns.end();
        ++feature_it, ++column_i)
      {
        if(feature_it->second >= feature_column_values.size())
        {
          Stream::Error ostr;
          ostr << "Line #" << line_i << ", column #" << feature_it->second << " out of parsed columns (" <<
            feature_column_values.size() << ")";
          throw Exception(ostr);
        }

        const std::string& value = feature_column_values[feature_it->second];

        if(feature_it->first == CTR::BF_CC_ID)
        {
          {
            // fill campaign
            auto it = ccid_to_campaign.find(value);
            if(it != ccid_to_campaign.end())
            {
              calc_params_filler.set_value(calc_params, CTR::BF_CAMPAIGN_ID, it->second);
            }
          }

          {
            // fill ccg
            auto it = ccid_to_ccg.find(value);
            if(it != ccid_to_ccg.end())
            {
              calc_params_filler.set_value(calc_params, CTR::BF_CCG_ID, it->second);
            }
          }
        }

        if(feature_it->first == CTR::BF_TAG_ID)
        {
        // fill publisher
          auto it = tag_to_publisher.find(value);
          if(it != tag_to_publisher.end())
          {
            calc_params_filler.set_value(calc_params, CTR::BF_PUBLISHER_ID, it->second);
          }
        }

        try
        {
          calc_params_filler.set_value(
            calc_params,
            feature_it->first,
            feature_column_values[feature_it->second]);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "Line #" << line_i <<
            ", column #" << feature_it->second <<
            ", feature_id = " << feature_it->first << ": " << ex.what();
          std::cerr << ostr.str() << std::endl;
          skip_line = true;
          break;
          //throw Exception(ostr);
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

    CTRGenerator::Calculation ctr_calculation;
    ctr_generator.calculate(ctr_calculation, calc_params);

    // output hashes
    std::ostringstream res_line_ostr;
    res_line_ostr << label;

    std::map<unsigned long, unsigned long> ordered_hashes;

    for(auto hash_it = ctr_calculation.hashes.begin();
      hash_it != ctr_calculation.hashes.end(); ++hash_it)
    {
      const uint32_t index = CTR::feature_hash_index(
        hash_it->first,
        features_size);
      const uint32_t svm_index = index + 1;
      if(allowed_feature_indexes.empty() || allowed_feature_indexes[svm_index])
      {
        ordered_hashes[svm_index] = hash_it->second;
      }
    }

    if(!feature_stats.empty())
    {
      const bool clicked = label == "1";
      ++total_impressions;
      total_clicks += clicked;
      for(const auto& hash : ordered_hashes)
      {
        FeatureStat& stat = feature_stats[hash.first];
        ++stat.impressions;
        stat.clicks += clicked;
      }
    }

    if(
      catboost_model &&
      line_i == 0 &&
      ordered_hashes.find(features_size) == ordered_hashes.end())
    {
      ordered_hashes[features_size] = 0;
    }

    for(auto hash_it = ordered_hashes.begin();
      hash_it != ordered_hashes.end(); ++hash_it)
    {
      res_line_ostr << ' ' << hash_it->first << ":" << hash_it->second;
    }

    if(dict_table.get())
    {
      // fill dict
      ctr_generator.fill_dictionary(*dict_table, calc_params);
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
      const uint32_t index = CTR::feature_hash_index(
        it->first,
        features_size);
      if(
        !allowed_feature_indexes.empty() &&
        !allowed_feature_indexes[index + 1])
      {
        continue;
      }
      dictionary_file << (index + 1) << ",";
      if(!it->second.empty())
      {
        std::string res = it->second;

        auto name_it = names.find(it->second);
        if(name_it != names.end())
        {
          res += "(";
          res += name_it->second;
          res += ")";
        }

        AdServer::LogProcessing::write_not_empty_string_as_csv(
          dictionary_file,
          res);
      }
      dictionary_file << std::endl;
    }
    dictionary_file.close();
  }

  if(!feature_stats.empty())
  {
    std::ofstream feature_stats_file(feature_stats_file_path);
    if(!feature_stats_file)
    {
      Stream::Error ostr;
      ostr << "Can't open feature stats file '" <<
        feature_stats_file_path << "'";
      throw Exception(ostr);
    }

    feature_stats_file << "0," << total_impressions << ',' <<
      total_clicks << '\n';
    for(std::size_t index = 1; index < feature_stats.size(); ++index)
    {
      const FeatureStat& stat = feature_stats[index];
      if(stat.impressions != 0)
      {
        feature_stats_file << index << ',' << stat.impressions << ',' <<
          stat.clicks << '\n';
      }
    }
  }
}

void
Application_::generate_xgb_ctr_(
  std::ostream& out,
  std::istream& in,
  const char* xgb_model_file,
  const char* config_file,
  const char* feature_columns_str,
  const char* cc_to_ccg_dictionary_file_path,
  const char* cc_to_campaign_dictionary_file_path,
  const char* tag_to_publisher_dictionary_file_path,
  bool out_hashes)
{
  using namespace xsd::AdServer;

  // load creative dictionary
  std::map<std::string, std::string> ccid_to_ccg;
  if(cc_to_ccg_dictionary_file_path[0])
  {
    load_dictionary_(ccid_to_ccg, cc_to_ccg_dictionary_file_path);
  }

  std::map<std::string, std::string> ccid_to_campaign;
  if(cc_to_campaign_dictionary_file_path[0])
  {
    load_dictionary_(ccid_to_campaign, cc_to_campaign_dictionary_file_path);
  }

  // load tag dictionary
  std::map<std::string, std::string> tag_to_publisher;
  if(tag_to_publisher_dictionary_file_path[0])
  {
    load_dictionary_(tag_to_publisher, tag_to_publisher_dictionary_file_path);
  }

  /*
  out.setf(std::ios::fixed, std::ios::floatfield);
  out.precision(17);
  */
  CTR::XGBoostPredictorPool_var xgb_pool(
    new CTR::XGBoostPredictorPool(String::SubString(xgb_model_file)));
  CTR::XGBoostPredictorPool::Predictor_var xgb_predictor =
    xgb_pool->get_predictor();

  const auto config = load_ctr_generator_feature_config(config_file);

  // parse model config
  const unsigned long dimension = config.features_dimension;
  const unsigned long features_size = 1UL << dimension;
  CTR::FeatureNameResolver feature_name_resolver;

  // parse columns
  std::map<unsigned long, unsigned long> feature_columns;
  unsigned long label_index = 0;

  {
    std::vector<std::string> feature_column_names;
    Commons::CsvReader::parse_line(
      feature_column_names,
      String::SubString(feature_columns_str));
    unsigned long column_i = 0;
    for(auto it = feature_column_names.begin(); it != feature_column_names.end(); ++it, ++column_i)
    {
      if(*it == "label" || *it == "Label")
      {
        label_index = column_i;
      }
      else if(*it == "timestamp" || *it == "Timestamp")
      {
        feature_columns[CalculateParamsFiller::BF_TIMESTAMP] = column_i;
      }
      else if(*it == "link" || *it == "Link")
      {
        feature_columns[CalculateParamsFiller::BF_LINK] = column_i;
      }
      else if(!it->empty() && (*it)[0] != '#')
      {
        String::AsciiStringManip::to_lower(*it);

        CTR::BasicFeature basic_feature;
        if(!feature_name_resolver.basic_feature_by_name(basic_feature, *it))
        {
          Stream::Error ostr;
          ostr << "Invalid basic feature name: '" << *it << "'";
          throw Exception(ostr);
        }

        feature_columns[basic_feature] = column_i;
      }
    }
  }

  // fetch input (values)
  CTRGenerator ctr_generator(config.features, true);
  CalculateParamsFiller calc_params_filler;

  unsigned long line_i = 0;
  std::vector<std::string> feature_column_values;
  feature_column_values.reserve(feature_columns.size());
  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    feature_column_values.resize(0);
    Commons::CsvReader::parse_line(feature_column_values, line);

    CTRGenerator::CalculateParams calc_params;
    std::string label = feature_column_values[label_index];

    bool skip_line = false;
    unsigned long column_i = 0;
    for(auto feature_it = feature_columns.begin();
      feature_it != feature_columns.end();
      ++feature_it, ++column_i)
    {
      if(feature_it->second >= feature_column_values.size())
      {
        Stream::Error ostr;
        ostr << "Line #" << line_i << ", column #" << feature_it->second << " out of parsed columns (" <<
          feature_column_values.size() << ")";
        throw Exception(ostr);
      }

      const std::string& value = feature_column_values[feature_it->second];

      if(feature_it->first == CTR::BF_CC_ID)
      {
        {
          // fill campaign
          auto it = ccid_to_campaign.find(value);
          if(it != ccid_to_campaign.end())
          {
            calc_params_filler.set_value(calc_params, CTR::BF_CAMPAIGN_ID, it->second);
          }
        }

        {
          // fill ccg
          auto it = ccid_to_ccg.find(value);
          if(it != ccid_to_ccg.end())
          {
            calc_params_filler.set_value(calc_params, CTR::BF_CCG_ID, it->second);
          }
        }
      }

      if(feature_it->first == CTR::BF_TAG_ID)
      {
        // fill publisher
        auto it = tag_to_publisher.find(value);
        if(it != tag_to_publisher.end())
        {
          calc_params_filler.set_value(calc_params, CTR::BF_PUBLISHER_ID, it->second);
        }
      }

      try
      {
        calc_params_filler.set_value(
          calc_params,
          feature_it->first,
          feature_column_values[feature_it->second]);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Line #" << line_i <<
          ", column #" << feature_it->second <<
          ", feature_id = " << feature_it->first << ": " << ex.what();
        std::cerr << ostr.str() << std::endl;
        skip_line = true;
        break;
        //throw Exception(ostr);
      }
    }

    if(skip_line)
    {
      continue;
    }

    CTRGenerator::Calculation ctr_calculation;
    ctr_generator.calculate(ctr_calculation, calc_params);

    // output hashes
    std::ostringstream res_line_ostr;
    res_line_ostr << label;

    CTR::HashArray hashes;
    for(auto hash_it = ctr_calculation.hashes.begin();
      hash_it != ctr_calculation.hashes.end(); ++hash_it)
    {
      const uint32_t index = CTR::feature_hash_index(
        hash_it->first,
        features_size);
      hashes.push_back(std::make_pair(index, hash_it->second));
    }

    const RevenueDecimal ctr = Generics::convert_float<RevenueDecimal>(xgb_predictor->predict(hashes));

    // push line to output
    if(out_hashes)
    {
      out << "hashes: ";
      for(auto hash_it = hashes.begin(); hash_it != hashes.end(); ++hash_it)
      {
        if(hash_it != hashes.begin())
        {
          out << ",";
        }
        out << hash_it->first;
      }
      out << std::endl;
    }

    out << /*line_i << ": " <<*/ ctr << std::endl;
    ++line_i;
  }
}

void
Application_::generate_ctr_(
  std::ostream& out,
  std::istream& in,
  const char* config_dir,
  const char* feature_columns_str,
  const char* cc_to_ccg_dictionary_file_path,
  const char* cc_to_campaign_dictionary_file_path,
  const char* tag_to_publisher_dictionary_file_path,
  bool out_hashes)
{
  using namespace xsd::AdServer;

  // load creative dictionary
  std::map<std::string, std::string> ccid_to_ccg;
  if(cc_to_ccg_dictionary_file_path[0])
  {
    load_dictionary_(ccid_to_ccg, cc_to_ccg_dictionary_file_path);
  }

  std::map<std::string, std::string> ccid_to_campaign;
  if(cc_to_campaign_dictionary_file_path[0])
  {
    load_dictionary_(ccid_to_campaign, cc_to_campaign_dictionary_file_path);
  }

  // load tag dictionary
  std::map<std::string, std::string> tag_to_publisher;
  if(tag_to_publisher_dictionary_file_path[0])
  {
    load_dictionary_(tag_to_publisher, tag_to_publisher_dictionary_file_path);
  }

  /*
  out.setf(std::ios::fixed, std::ios::floatfield);
  out.precision(17);
  */
  CTR::CTRProvider_var ctr_provider(
    new CTR::CTRProviderImpl(String::SubString(config_dir), Generics::Time::ZERO, nullptr));

  // parse columns
  CTR::FeatureNameResolver feature_name_resolver;
  std::map<unsigned long, unsigned long> feature_columns;
  unsigned long label_index = 0;

  {
    std::vector<std::string> feature_column_names;
    Commons::CsvReader::parse_line(
      feature_column_names,
      String::SubString(feature_columns_str));
    unsigned long column_i = 0;
    for(auto it = feature_column_names.begin(); it != feature_column_names.end(); ++it, ++column_i)
    {
      if(*it == "label" || *it == "Label")
      {
        label_index = column_i;
      }
      else if(*it == "timestamp" || *it == "Timestamp")
      {
        feature_columns[CalculateParamsFiller::BF_TIMESTAMP] = column_i;
      }
      else if(*it == "link" || *it == "Link")
      {
        feature_columns[CalculateParamsFiller::BF_LINK] = column_i;
      }
      else if(!it->empty() && (*it)[0] != '#')
      {
        String::AsciiStringManip::to_lower(*it);

        CTR::BasicFeature basic_feature;
        if(!feature_name_resolver.basic_feature_by_name(basic_feature, *it))
        {
          Stream::Error ostr;
          ostr << "Invalid basic feature name: '" << *it << "'";
          throw Exception(ostr);
        }

        feature_columns[basic_feature] = column_i;
      }
    }
  }

  // fetch input (values)
  CalculateParamsFiller calc_params_filler;
  Holder holder;

  unsigned long line_i = 0;
  std::vector<std::string> feature_column_values;
  feature_column_values.reserve(feature_columns.size());
  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    feature_column_values.resize(0);
    Commons::CsvReader::parse_line(feature_column_values, line);

    CTRGenerator::CalculateParams calc_params;
    std::string label = feature_column_values[label_index];

    bool skip_line = false;
    unsigned long column_i = 0;
    for(auto feature_it = feature_columns.begin();
      feature_it != feature_columns.end();
      ++feature_it, ++column_i)
    {
      if(feature_it->second >= feature_column_values.size())
      {
        Stream::Error ostr;
        ostr << "Line #" << line_i << ", column #" << feature_it->second << " out of parsed columns (" <<
          feature_column_values.size() << ")";
        throw Exception(ostr);
      }

      const std::string& value = feature_column_values[feature_it->second];

      if(feature_it->first == CTR::BF_CC_ID)
      {
        {
          // fill campaign
          auto it = ccid_to_campaign.find(value);
          if(it != ccid_to_campaign.end())
          {
            calc_params_filler.set_value(calc_params, CTR::BF_CAMPAIGN_ID, it->second);
          }
        }

        {
          // fill ccg
          auto it = ccid_to_ccg.find(value);
          if(it != ccid_to_ccg.end())
          {
            calc_params_filler.set_value(calc_params, CTR::BF_CCG_ID, it->second);
          }
        }
      }

      if(feature_it->first == CTR::BF_TAG_ID)
      {
        // fill publisher
        auto it = tag_to_publisher.find(value);
        if(it != tag_to_publisher.end())
        {
          calc_params_filler.set_value(calc_params, CTR::BF_PUBLISHER_ID, it->second);
        }
      }

      try
      {
        calc_params_filler.set_value(
          calc_params,
          feature_it->first,
          feature_column_values[feature_it->second]);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Line #" << line_i <<
          ", column #" << feature_it->second <<
          ", feature_id = " << feature_it->first << ": " << ex.what();
        std::cerr << ostr.str() << std::endl;
        skip_line = true;
        break;
        //throw Exception(ostr);
      }
    }

    if(skip_line)
    {
      continue;
    }

    // calc_params(CTRGenerator::CalculateParams) filled
    // convert it to CampaignSelectParams
    Generics::MonoAllocatorArena request_params_arena;
    CampaignSelectParams_var request_params_ptr(new CampaignSelectParams(
      true, // profiling_available
      FreqCapIdSet(&request_params_arena),
      SeqOrderMap(&request_params_arena),
      0, // colocation
      0, // tag
      Tag::SizeMap(),
      false,
      -1, // visibility
      -1, // viewability
      &request_params_arena
      ));

    CampaignSelectParams& request_params = *request_params_ptr;

    init_campaign_select_params(
      holder,
      request_params,
      calc_params.colo_id, // colo_id
      calc_params.publisher_id, // publisher_id
      calc_params.tag_id,  // tag_id
      calc_params.size_id,
      "" // size name
      );

    Campaign_var campaign;
    Creative_var creative = create_creative(
      campaign,
      calc_params.advertiser_id, // account_id
      calc_params.advertiser_id,
      calc_params.campaign_id,
      calc_params.ccg_id,  // ccg_id
      calc_params.creative_id,
      calc_params.cc_id, // cc_id
      calc_params.size_id,
      calc_params.content_categories,
      calc_params.visual_categories
      );

    request_params.ext_tag_id = calc_params.etag;
    request_params.short_referer_hash = calc_params.referer_hash;
    request_params.time_hour = calc_params.hour;
    request_params.time_week_day = calc_params.wd;
    request_params.last_platform_channel_id = calc_params.device_id;

    if(calc_params.campaign_freq > 0)
    {
      request_params.campaign_imps.insert(
        std::make_pair(calc_params.campaign_id, calc_params.campaign_freq));
    }

    // fill channels
    std::copy(
      calc_params.channels.begin(),
      calc_params.channels.end(),
      std::inserter(request_params.channels, request_params.channels.begin()));
    std::copy(
      calc_params.geo_channels.begin(),
      calc_params.geo_channels.end(),
      std::inserter(request_params.channels, request_params.channels.begin()));

    std::copy(
      calc_params.geo_channels.begin(),
      calc_params.geo_channels.end(),
      std::inserter(request_params.geo_channels, request_params.geo_channels.begin()));

    request_params.tag_visibility = calc_params.tag_visibility;
    request_params.tag_predicted_viewability = calc_params.tag_predicted_viewability;
    request_params.ssp_tag_id = calc_params.ssp_tag_id;
    request_params.ssp_ctr = calc_params.ssp_ctr;
    request_params.ssp_viewability = calc_params.ssp_viewability;
    request_params.ssp_vtr = calc_params.ssp_vtr;

    RevenueDecimal ctr = RevenueDecimal::ZERO;

    {
      // eval ctr
      CTR::CTRProvider::Calculation_var calculation =
        ctr_provider->create_calculation(request_params_ptr);

      if(calculation.in())
      {
        CTR::CTRProvider::CalculationContext_var calculation_context =
          calculation->create_context(request_params.tag->sizes.begin()->second);

        ctr = calculation_context->get_ctr(creative);

        if(out_hashes)
        {
          CTR::HashArray hashes;
          const auto* impl_calculation_context =
            dynamic_cast<const CTR::CTRProviderImpl::CalculationContext*>(
              calculation_context.in());
          if(!impl_calculation_context)
          {
            Stream::Error ostr;
            ostr << "Unexpected CTR calculation context type";
            throw Exception(ostr);
          }
          impl_calculation_context->get_xgb_hashes_i(hashes, creative);
          std::sort(hashes.begin(), hashes.end());

          out << "hashes: ";
          for(auto hash_it = hashes.begin(); hash_it != hashes.end(); ++hash_it)
          {
            if(hash_it != hashes.begin())
            {
              out << ",";
            }
            out << hash_it->first << ':' << hash_it->second;
          }
          out << std::endl;
        }
      }
    }
    /*
    else
    {
      std::cout << "selected default ctr algorithm" << std::endl;
    }
    */

    // push line to output
    out << /*line_i << ": " << */ ctr << std::endl;
    ++line_i;
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
