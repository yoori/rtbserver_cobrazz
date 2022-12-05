#include <list>
#include <vector>
#include <array>
#include <iterator>
#include <iostream>
#include <fstream>

#include <Generics/MemBuf.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>

#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <LogCommons/CsvUtils.hpp>

#include <Commons/Algs.hpp>
#include <Commons/PathManip.hpp>
#include <Commons/FileManip.hpp>
#include <Commons/CsvReader.hpp>
#include <TestCommons/ActiveObjectCallback.hpp>

#include "SegmentProfile.hpp"
#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "\nUsage: \nSegmentUtil <COMMAND> <PARAMS>\n"
    "Commands: \n"
    "  profile\n"
    "  request\n"
    "  print\n"
    "  get-segments\n";
}

struct SegmentMatchesLess
{
  bool
  operator()(
    const Profiles::SegmentMatchesReader& segment_matches,
    unsigned long segment_id)
    const
  {
    return segment_matches.segment_id() < segment_id;
  }

  bool
  operator()(
    unsigned long segment_id,
    const Profiles::SegmentMatchesReader& segment_matches)
    const
  {
    return segment_id < segment_matches.segment_id();
  }
};

struct SegmentMatchesMerge
{
  SegmentMatchesMerge(const Generics::Time& ts)
    : timestamp_(ts)
  {}

  Profiles::SegmentMatchesWriter
  operator()(
    const Profiles::SegmentMatchesReader& reader,
    unsigned long /*segment_id*/)
    const
  {
    Profiles::SegmentMatchesWriter res;

    res.segment_id() = reader.segment_id();
    auto ins_ts_it = std::lower_bound(
      reader.timestamps().begin(),
      reader.timestamps().end(),
      timestamp_.tv_sec);
    res.timestamps().reserve(reader.timestamps().size() + 1);
    std::copy(
      reader.timestamps().begin(),
      ins_ts_it,
      std::back_inserter(res.timestamps()));
    res.timestamps().push_back(timestamp_.tv_sec);
    std::copy(
      ins_ts_it,
      reader.timestamps().end(),
      std::back_inserter(res.timestamps()));

    return res;
  }

  Profiles::SegmentMatchesWriter
  operator()(
    unsigned long segment_id,
    const Profiles::SegmentMatchesReader& reader)
    const
  {
    return this->operator()(reader, segment_id);
  }

protected:
  const Generics::Time timestamp_;
};

struct SegmentMatchesConverter
{
  SegmentMatchesConverter(const Generics::Time& ts)
    : timestamp_(ts)
  {}

  const Profiles::SegmentMatchesWriter&
  operator()(
    const Profiles::SegmentMatchesWriter& segment_matches)
    const
  {
    return segment_matches;
  }

  Profiles::SegmentMatchesWriter
  operator()(
    const Profiles::SegmentMatchesReader& segment_matches)
    const
  {
    Profiles::SegmentMatchesWriter res;
    res.segment_id() = segment_matches.segment_id();
    res.timestamps().reserve(segment_matches.timestamps().size());
    std::copy(
      segment_matches.timestamps().begin(),
      segment_matches.timestamps().end(),
      std::back_inserter(res.timestamps()));
    return segment_matches;
  }

  Profiles::SegmentMatchesWriter
  operator()(unsigned long segment_id)
    const
  {
    Profiles::SegmentMatchesWriter res;
    res.segment_id() = segment_id;
    res.timestamps().push_back(timestamp_.tv_sec);    
    return res;
  }

protected:
  const Generics::Time timestamp_;
};

Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

uint64_t
string_to_hash(const std::string& value)
{
  uint64_t res;

  {
    Generics::Murmur64Hash hasher(res);
    hasher.add(value.data(), value.size());
  }

  return res;
}

ReferenceCounting::SmartPtr<
  AdServer::ProfilingCommons::TransactionProfileMap<Generics::NumericHashAdapter<uint64_t> > >
open_storage(
  Generics::ActiveObject_var& activator,
  const char* filename,
  unsigned long level0_size)
  /*throw(eh::Exception)*/
{
  std::string root;
  std::string prefix;
  AdServer::PathManip::split_path(filename, &root, &prefix, false);

  auto user_map = AdServer::ProfilingCommons::ProfileMapFactory::open_transaction_level_map<
    Generics::NumericHashAdapter<uint64_t>,
    AdServer::ProfilingCommons::NumericSerializer>(
      activator,
      Generics::ActiveObjectCallback_var(
        new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, "SegmentUtil")),
      root.c_str(),
      prefix.c_str(),
      AdServer::ProfilingCommons::LevelMapTraits(
        AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
        10*1024*1024,
        level0_size, // level 0 size
        level0_size * 2, // max undumped size
        3, // max levels 0
        Generics::Time::ZERO));

  activator->activate_object();

  return user_map;
}

void
Application_::parse_hashes_(
  std::vector<uint32_t>& res,
  const String::SubString& str)
{
  String::StringManip::Splitter<
    String::AsciiStringManip::SepSpace> tokenizer(str);

  String::SubString token;
  while(tokenizer.get_token(token))
  {
    if(!token.empty())
    {
      uint32_t feature_id;
      if(!String::StringManip::str_to_int(token, feature_id))
      {
        Stream::Error ostr;
        ostr << "can't parse feature_id value '" << token << "'";
        throw Exception(ostr);
      }

      res.push_back(feature_id);
    }
  }
}

void
Application_::load_hash_dictionary_(
  Dictionary& dict,
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

    parse_hashes_(dict[values[0]], values[1]);
  }
}

void
Application_::save_hash_name_dictionary_(
  const char* file_name,
  const HashNameDictionary& dict)
{
  std::ofstream file(file_name);
  for(auto dict_it = dict.begin(); dict_it != dict.end(); ++dict_it)
  {
    file << dict_it->first << ",";
    AdServer::LogProcessing::write_string_as_csv(file, dict_it->second);
    file << std::endl;
  }
}

void
Application_::load_hash_name_dictionary_(
  HashNameDictionary& dict,
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

    uint32_t feature_id;
    if(!String::StringManip::str_to_int(values[0], feature_id))
    {
      Stream::Error ostr;
      ostr << "can't parse feature_id value in dictionary: '" << values[0] << "'";
      throw Exception(ostr);
    }

    dict[feature_id] = values[1];
  }
}

void
Application_::make_profile_(
  std::istream& in,
  const char* filename,
  const char* columns,
  unsigned long level0_size)
  /*throw(eh::Exception)*/
{
  // parse column heads
  std::vector<std::string> values;
  std::vector<std::pair<unsigned long, std::unique_ptr<Dictionary> > > dictionaries;
  unsigned long id_column_i = 0;
  unsigned long timestamp_column_i = 0;
  std::vector<unsigned long> hash_columns;

  {
    // parse column heads
    Commons::CsvReader::parse_line(values, String::SubString(columns));

    unsigned long column_i = 0;
    for(auto it = values.begin(); it != values.end(); ++it, ++column_i)
    {
      if(*it == "id")
      {
        id_column_i = column_i;
      }
      else if(*it == "timestamp")
      {
        timestamp_column_i = column_i;
      }
      else if(*it == "hashes")
      {
        hash_columns.push_back(column_i);
      }
      else if(it->compare(0, 4, "ref:") == 0)
      {
        std::string file(*it, 4);
        std::pair<unsigned long, std::unique_ptr<Dictionary> > dict;
        dict.first = column_i;
        dict.second.reset(new Dictionary());
        load_hash_dictionary_(*dict.second, file.c_str());
        dictionaries.push_back(std::move(dict));
      }
    }
  }

  //const Generics::Time base_time = Generics::Time::ZERO;

  Generics::ActiveObject_var activator;
  auto user_map = open_storage(activator, filename, level0_size);

  std::string line;
  unsigned long line_i = 0;
  unsigned long timestamp_count = 0;
  unsigned long max_timestamp_count = 0;

  while(!in.eof())
  {
    std::getline(in, line);
    ++line_i;

    if(line.empty())
    {
      break;
    }

    values.clear();
    Commons::CsvReader::parse_line(values, line);

    std::string hashes_line;
    for(auto hash_column_it = hash_columns.begin();
      hash_column_it != hash_columns.end(); ++hash_column_it)
    {
      if(!hashes_line.empty() && *hashes_line.rbegin() != ' ')
      {
        hashes_line += ' ';
      }
      hashes_line += values[*hash_column_it];
    }

    String::StringManip::Splitter<
      String::AsciiStringManip::SepSpace> tokenizer(hashes_line);

    const std::string user_id = values[id_column_i];
    Generics::Time timestamp;

    {
      uint32_t ts;
      if(!String::StringManip::str_to_int(values[timestamp_column_i], ts))
      {
        Stream::Error ostr;
        ostr << "can't parse timestamp value '" << values[timestamp_column_i] << "'";
        throw Exception(ostr);
      }

      timestamp = Generics::Time(ts);
    }

    //Generics::Time timestamp = values[timestamp_column_i];

    std::vector<uint32_t> features;

    for(auto hash_column_it = hash_columns.begin(); hash_column_it != hash_columns.end(); ++hash_column_it)
    {
      parse_hashes_(features, values[*hash_column_it]);
    }

    for(auto dict_it = dictionaries.begin(); dict_it != dictionaries.end(); ++dict_it)
    {
      auto dict_value_it = dict_it->second->find(values[dict_it->first]);
      if(dict_value_it != dict_it->second->end())
      {
        std::copy(
          dict_value_it->second.begin(),
          dict_value_it->second.end(),
          std::back_inserter(features));
      }
    }

    std::sort(features.begin(), features.end());
    auto er_it = std::unique(features.begin(), features.end());
    features.erase(er_it, features.end());

    /*
    std::cerr << "request for " << user_id << ": ";
    Algs::print(std::cerr, features.begin(), features.end());
    std::cerr << std::endl;
    */

    Generics::ConstSmartMemBuf_var mb = user_map->get_profile(string_to_hash(user_id));
    Profiles::SegmentProfileWriter profile_writer;

    if(mb.in())
    {
      Profiles::SegmentProfileReader profile_reader(mb->membuf().data(), mb->membuf().size());

      profile_writer.segment_matches().reserve(profile_reader.segment_matches().size() + features.size());

      Algs::merge_unique(
        profile_reader.segment_matches().begin(),
        profile_reader.segment_matches().end(),
        features.begin(),
        features.end(),
        Algs::modify_inserter(
          std::back_inserter(profile_writer.segment_matches()),
          SegmentMatchesConverter(timestamp)),
        SegmentMatchesLess(),
        SegmentMatchesMerge(timestamp));
    }
    else
    {
      profile_writer.segment_matches().reserve(features.size());

      std::copy(
        features.begin(),
        features.end(),
        Algs::modify_inserter(
          std::back_inserter(profile_writer.segment_matches()),
          SegmentMatchesConverter(timestamp)));
    }

    // save profile
    unsigned long sz = profile_writer.size();

    Generics::SmartMemBuf_var mem_buf = new Generics::SmartMemBuf(sz);
    profile_writer.save(mem_buf->membuf().data(), sz);

    //print_profile_from_buf_(user_id.c_str(), mem_buf->membuf());

    Generics::ConstSmartMemBuf_var c_mem_buf = Generics::transfer_membuf(mem_buf);

    user_map->save_profile(
      string_to_hash(user_id),
      c_mem_buf,
      Generics::Time::ZERO,
      AdServer::ProfilingCommons::OP_RUNTIME);

    timestamp_count += features.size();
    max_timestamp_count = std::max(
      max_timestamp_count,
      profile_writer.segment_matches().size());

    if(line_i % 50000 == 0)
    {
      std::cerr << Generics::Time::get_time_of_day().gm_ft() <<
        ": processed " << line_i << " lines"
        ", avg timestamps = " << (timestamp_count / line_i) <<
        ", max timestamps = " << max_timestamp_count <<
        std::endl;
    }
  }

  //user_map->dump();

  activator->deactivate_object();
  activator->wait_object();
}

void
Application_::request_profile_(
  std::istream& in,
  const char* filename,
  const char* columns,
  const SegmentRuleArray& segment_config,
  HashNameDictionary* res_hash_dictionary,
  const HashNameDictionary* base_hash_dictionary,
  bool svm_format,
  std::unordered_set<unsigned long>* filter_features,
  unsigned long dimension)
  /*throw(eh::Exception)*/
{
  // parse column heads
  std::vector<std::string> values;
  unsigned long id_column_i = 0;
  unsigned long timestamp_column_i = 0;
  std::vector<unsigned long> push_columns;
  std::unordered_map<unsigned long, HashDescr> res_hash_descr_dictionary;

  {
    // parse column heads
    Commons::CsvReader::parse_line(values, String::SubString(columns));

    unsigned long column_i = 0;
    for(auto it = values.begin(); it != values.end(); ++it, ++column_i)
    {
      std::string col = *it;

      if(!col.empty() && col[0] == '+')
      {
        push_columns.push_back(column_i);
        col = std::string(col, 1);
      }

      if(col == "id")
      {
        id_column_i = column_i;
      }
      else if(col == "timestamp")
      {
        timestamp_column_i = column_i;
      }
    }
  }

  //const Generics::Time base_time = Generics::Time::ZERO;

  Generics::ActiveObject_var activator;

  auto user_map = open_storage(activator, filename, 5);

  std::set<Generics::Time> times_order;
  for(auto rule_it = segment_config.begin(); rule_it != segment_config.end(); ++rule_it)
  {
    times_order.insert(rule_it->time_to);
  }

  std::vector<Generics::Time> TIMES(times_order.begin(), times_order.end());

  const Generics::Time MAX_IN_TIMES = *std::max_element(TIMES.begin(), TIMES.end());

  std::vector<unsigned long> time_counters(TIMES.size(), 0);
  std::string line;

  while(!in.eof())
  {
    std::getline(in, line);
    if(line.empty())
    {
      break;
    }

    values.clear();
    Commons::CsvReader::parse_line(values, line);

    const std::string user_id = values[id_column_i];
    Generics::Time timestamp;

    {
      uint32_t ts;
      if(!String::StringManip::str_to_int(values[timestamp_column_i], ts))
      {
        Stream::Error ostr;
        ostr << "can't parse timestamp value '" << values[timestamp_column_i] << "'";
        throw Exception(ostr);
      }

      timestamp = Generics::Time(ts);
    }

    Generics::ConstSmartMemBuf_var mb = user_map->get_profile(string_to_hash(user_id));
    Profiles::SegmentProfileReader profile_reader(
      mb->membuf().data(),
      mb->membuf().size());

    std::vector<uint32_t> hashes;

    for(auto segment_it = profile_reader.segment_matches().begin();
      segment_it != profile_reader.segment_matches().end(); ++segment_it)
    {
      std::fill(time_counters.begin(), time_counters.end(), 0);

      auto ts_it = std::upper_bound(
        (*segment_it).timestamps().begin(),
        (*segment_it).timestamps().end(),
        timestamp.tv_sec + 1);

      auto cur_time_it = TIMES.begin();

      for(Profiles::SegmentMatchesReader::timestamps_Container::const_reverse_iterator rev_ts_it(ts_it);
        rev_ts_it != (*segment_it).timestamps().rend() && cur_time_it != TIMES.end();
        ++rev_ts_it)
      {
        if(timestamp - *rev_ts_it > *cur_time_it)
        {
          ++cur_time_it;
        }

        ++time_counters[cur_time_it - TIMES.begin()];

        /*
        std::cout << "ADD TS[" << (cur_time_it - TIMES.begin()) << "]: " <<
          "ts = " << *rev_ts_it <<
          ", cur_time_it = " << *cur_time_it <<
          ", timestamp = " << timestamp.tv_sec <<
          "(" << (*segment_it).segment_id() << ")" << std::endl;
        */
      }

      /*
      unsigned long i = 0;
      for(auto tc_it = time_counters.begin(); tc_it != time_counters.end(); ++tc_it, ++i)
      {
        std::cout << "TS[" << i << "]: " << *tc_it << "(" << (*segment_it).segment_id() << ")" << std::endl;
      }
      */

      unsigned long cur_sum = 0;
      for(auto counter_it = time_counters.begin(); counter_it != time_counters.end(); ++counter_it)
      {
        cur_sum += *counter_it;

        for(auto rule_it = segment_config.begin(); rule_it != segment_config.end(); ++rule_it)
        {
          if(rule_it->time_to >= TIMES[counter_it - time_counters.begin()] &&
            rule_it->min_visits <= cur_sum)
          {
            uint32_t seed = rule_it->min_visits * 10000000 + rule_it->time_to.tv_sec;
            Generics::Murmur32v3Hasher hash_adapter(seed);
            uint32_t segment_id = (*segment_it).segment_id();
            hash_adapter.add(&segment_id, sizeof(segment_id));

            uint32_t res_hash = hash_adapter.finalize();

            res_hash = res_hash >> (32 - dimension);

            if(res_hash_dictionary)
            {
              res_hash_descr_dictionary.insert(
                std::make_pair(res_hash, HashDescr(segment_id, &*rule_it)));
            }

            if(!filter_features || filter_features->find(res_hash) != filter_features->end())
            {
              hashes.push_back(res_hash);
            }

            /*
            std::cerr << "WORK #" << segment_id << ":" << rule_it->min_visits << " per " <<
              TIMES[counter_it - time_counters.begin()] << " sec => " <<
              hashes.back() <<
              "(seed = " << seed <<
              ", time_to = " << rule_it->time_to <<
              ", mv = " << rule_it->min_visits <<
              ", cur_sum = " << cur_sum << ")" <<
              std::endl;
            */
          }
        }
      }
    }

    std::sort(hashes.begin(), hashes.end());
    auto er_it = std::unique(hashes.begin(), hashes.end());
    hashes.erase(er_it, hashes.end());

    const char SEP = (svm_format ? ' ' : ',');

    //std::cout << "hashes.size() = " << hashes.size() << std::endl;
    for(auto push_it = push_columns.begin(); push_it != push_columns.end(); ++push_it)
    {
      AdServer::LogProcessing::write_string_as_csv(std::cout, values[*push_it]);
      std::cout << SEP;
    }

    if(!svm_format)
    {
      Algs::print(std::cout, hashes.begin(), hashes.end(), " ");
    }
    else
    {
      for(auto hash_it = hashes.begin(); hash_it != hashes.end(); ++hash_it)
      {
        if(hash_it != hashes.begin())
        {
          std::cout << SEP;
        }

        std::cout << *hash_it << ":1";
      }
    }

    std::cout << std::endl;
    // TODO : print result
  }

  //user_map->dump();

  if(res_hash_dictionary)
  {
    for(auto hash_it = res_hash_descr_dictionary.begin();
      hash_it != res_hash_descr_dictionary.end(); ++hash_it)
    {
      std::ostringstream name_ostr;

      if(base_hash_dictionary)
      {
        auto hash_name_it = base_hash_dictionary->find(hash_it->second.base_hash);
        if(hash_name_it != base_hash_dictionary->end())
        {
          name_ostr << hash_name_it->second;
        }
      }

      name_ostr << "(v=" << hash_it->second.rule->min_visits <<
        ",t=" << hash_it->second.rule->time_to.tv_sec << ")";

      res_hash_dictionary->insert(
        std::make_pair(hash_it->first, name_ostr.str()));
    }
  }
  
  activator->deactivate_object();
  activator->wait_object();
}

void
Application_::print_profile_(
  const char* user_id,
  const char* filename)
  /*throw(eh::Exception)*/
{
  Generics::ActiveObject_var activator;
  auto user_map = open_storage(activator, filename, 5);

  if(user_id[0])
  {
    Generics::ConstSmartMemBuf_var mb = user_map->get_profile(string_to_hash(user_id));

    if(!mb)
    {
      std::cerr << "Not found" << std::endl;
    }
    else
    {
      print_profile_from_buf_(user_id, mb->membuf());
    }
  }

  activator->deactivate_object();
  activator->wait_object();
}

void
Application_::print_profile_from_buf_(
  const char* user_id,
  const Generics::MemBuf& buf)
  noexcept
{
  try
  {
    // print content
    Profiles::SegmentProfileReader profile_reader(
      buf.data(), buf.size());

    std::cout << "User: " << user_id << ":" << std::endl;
    for(auto segment_it = profile_reader.segment_matches().begin();
      segment_it != profile_reader.segment_matches().end(); ++segment_it)
    {
      std::cout << "  " << (*segment_it).segment_id() << ": ";
      Algs::print(std::cout, (*segment_it).timestamps().begin(), (*segment_it).timestamps().end());
      std::cout << std::endl;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Can't print profile for User '" << user_id <<
      "': " << ex.what() << std::endl;
  }
}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::CheckOption opt_plain;
  Generics::AppUtils::StringOption opt_user_id("");
  Generics::AppUtils::StringOption opt_storage_root("./");
  Generics::AppUtils::Option<unsigned long> opt_level0_size(5);
  Generics::AppUtils::StringOption opt_segment_config("0:1");
  Generics::AppUtils::StringOption opt_base_dictionary("");
  Generics::AppUtils::StringOption opt_res_dictionary_file("");
  Generics::AppUtils::CheckOption opt_svm_format;
  Generics::AppUtils::StringOption opt_features_file;
  Generics::AppUtils::Option<unsigned long> opt_dimension(16);

  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);
  args.add(
    Generics::AppUtils::equal_name("plain") ||
    Generics::AppUtils::short_name("p"),
    opt_plain);
  args.add(
    Generics::AppUtils::equal_name("user-id") ||
    Generics::AppUtils::equal_name("uid") ||
    Generics::AppUtils::short_name("u"),
    opt_user_id);
  args.add(
    Generics::AppUtils::equal_name("storage") ||
    Generics::AppUtils::short_name("s"),
    opt_storage_root);
  args.add(
    Generics::AppUtils::equal_name("level0-size") ||
    Generics::AppUtils::short_name("l0"),
    opt_level0_size);
  args.add(
    Generics::AppUtils::equal_name("segment-config") ||
    Generics::AppUtils::short_name("sc"),
    opt_segment_config);
  args.add(
    Generics::AppUtils::equal_name("base-dict") ||
    Generics::AppUtils::short_name("bd"),
    opt_base_dictionary);
  args.add(
    Generics::AppUtils::equal_name("dict") ||
    Generics::AppUtils::short_name("d"),
    opt_res_dictionary_file);
  args.add(
    Generics::AppUtils::equal_name("svm"),
    opt_svm_format);
  args.add(
    Generics::AppUtils::equal_name("features-file") ||
    Generics::AppUtils::short_name("ff"),
    opt_features_file);
  args.add(
    Generics::AppUtils::equal_name("dimension"),
    opt_dimension);

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

  if(command == "profile")
  {
    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "columns not defined" << std::endl;
      return;
    }

    const std::string columns = *command_it;

    make_profile_(
      std::cin,
      opt_storage_root->c_str(),
      columns.c_str(),
      *opt_level0_size * 1024 * 1024 * 1024);
  }
  else if(command == "request")
  {
    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "columns not defined" << std::endl;
      return;
    }

    const std::string columns = *command_it;

    SegmentRuleArray segment_config;

    {
      // parse segment config
      String::StringManip::Splitter<
        String::AsciiStringManip::SepComma> tokenizer(*opt_segment_config);

      String::SubString token;
      while(tokenizer.get_token(token))
      {
        if(!token.empty())
        {
          String::StringManip::Splitter<
            String::AsciiStringManip::SepColon> sub_tokenizer(token);

          String::SubString time_to_str;
          sub_tokenizer.get_token(time_to_str);

          String::SubString min_visits_str;
          sub_tokenizer.get_token(min_visits_str);

          unsigned long time_to;
          if(!String::StringManip::str_to_int(time_to_str, time_to))
          {
            Stream::Error ostr;
            ostr << "can't parse time_to value '" << time_to_str << "'";
            throw Exception(ostr);
          }

          unsigned long min_visits;
          if(!String::StringManip::str_to_int(min_visits_str, min_visits))
          {
            Stream::Error ostr;
            ostr << "can't parse min_visits value '" << min_visits_str << "'";
            throw Exception(ostr);
          }

          SegmentRule segment_rule;
          segment_rule.time_to = Generics::Time(time_to);
          segment_rule.min_visits = min_visits;
          segment_config.push_back(segment_rule);
        }
      }
    }

    std::unique_ptr<HashNameDictionary> base_dictionary;

    if(!opt_base_dictionary->empty())
    {
      base_dictionary.reset(new HashNameDictionary());
      load_hash_name_dictionary_(*base_dictionary, opt_base_dictionary->c_str());
    }

    std::unique_ptr<HashNameDictionary> res_dictionary;

    if(!opt_res_dictionary_file->empty())
    {
      res_dictionary.reset(new HashNameDictionary());
      if(AdServer::FileManip::file_exists(*opt_res_dictionary_file))
      {
        load_hash_name_dictionary_(*res_dictionary, opt_res_dictionary_file->c_str());
      }
    }

    std::unordered_set<unsigned long> filter_features;

    if(!opt_features_file->empty())
    {
      std::ifstream ff(opt_features_file->c_str());
      if(!ff.is_open())
      {
        Stream::Error ostr;
        ostr << "can't open filter features file '" << *opt_features_file << "'";
        throw Exception(ostr);
      }

      while(!ff.eof())
      {
        std::string line;
        std::getline(ff, line);

        if(!line.empty())
        {
          unsigned long feature_id;
          if(!String::StringManip::str_to_int(line, feature_id))
          {
            Stream::Error ostr;
            ostr << "can't parse filter feature '" << line << "'";
            throw Exception(ostr);
          }

          filter_features.insert(feature_id);
        }
      }
    }

    request_profile_(
      std::cin,
      opt_storage_root->c_str(),
      columns.c_str(),
      segment_config,
      res_dictionary.get(),
      base_dictionary.get(),
      opt_svm_format.enabled(),
      !filter_features.empty() ? &filter_features : 0,
      *opt_dimension);

    if(res_dictionary.get())
    {
      save_hash_name_dictionary_(opt_res_dictionary_file->c_str(), *res_dictionary);
    }
  }
  else if(command == "print")
  {
    print_profile_(opt_user_id->c_str(), opt_storage_root->c_str());
  }
  else if(command == "get-segments")
  {
  }
  else if(command == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }
  else
  {
    std::cerr << "Unknown command '" << command << "'. "
      "See help for more info." << std::endl;
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

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

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


