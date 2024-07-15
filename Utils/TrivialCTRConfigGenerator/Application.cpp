#include <list>
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>

#include <Generics/MemBuf.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/GnuHashTable.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>

#include <Commons/Algs.hpp>
#include <Commons/Containers.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/IPCrypter.hpp>
#include <Commons/UserInfoManip.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "Usage: TrivialCTRConfigGenerator <COMMAND> [OPTIONS]\n"
    "Commands:\n"
    "  generate <FOLDER>\n"
    "Options:\n"
    "  --output: source id of SSP (empty by default)\n"
    "Sample: \n"
    "  \n";

  const String::SubString EMPTY_DOMAIN_MARKER("-");
  const unsigned long TRUST_IMPS = 10000;
}

// DirSelectAdapter
class DirSelectAdapter
{
public:
  DirSelectAdapter(std::set<std::string>& files)
    noexcept;

  void
  operator() (
    const char* file_name,
    const struct stat&);

private:
  std::set<std::string>& files_;
};

DirSelectAdapter::DirSelectAdapter(
  std::set<std::string>& files)
  noexcept
  : files_(files)
{}

void
DirSelectAdapter::operator()(
  const char* file_name,
  const struct stat&)
{
  files_.insert(file_name);
}

// Application
Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

int
Application_::main(int& argc, char** argv)
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::StringOption opt_output;

  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);
  args.add(
    Generics::AppUtils::equal_name("output"),
    opt_output);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  auto cmd_it = commands.begin();
  std::string command = *cmd_it;

  if(command == "generate")
  {
    ++cmd_it;
    if(cmd_it == commands.end())
    {
      Stream::Error ostr;
      ostr << "generate: input folder isn't defined";
      throw InvalidArgument(ostr);
    }
    auto input_folder = *cmd_it;

    ++cmd_it;
    if(cmd_it == commands.end())
    {
      Stream::Error ostr;
      ostr << "generate: output folder isn't defined";
      throw InvalidArgument(ostr);
    }
    auto output_file = *cmd_it;

    generate_(input_folder, output_file);
  }
  else if(command == "help")
  {
    std::cout << USAGE << std::endl;
    return 0;
  }
  else
  {
    std::cerr << "Unknown command '" << command << "'. "
      "See help for more info." << std::endl;
  }

  return 0;
}

struct Key
{
public:
  Key(unsigned long tag_id_val, std::string domain_val)
    : tag_id(tag_id_val),
      domain(std::move(domain_val)),
      hash_(0)
  {
    calc_hash_();
  }

  bool operator==(const Key& rhs) const
  {
    return tag_id == rhs.tag_id &&
      domain == rhs.domain;
  }
  
  size_t hash() const
  {
    return hash_;
  }

  const unsigned long tag_id;
  const std::string domain;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_, 0);
    hash_add(hasher, tag_id);
    hash_add(hasher, domain);
  }

private:
  size_t hash_;
};

struct Value
{
  Value(): imps(0), clicks(0)
  {}

  Value(unsigned long imps_val, unsigned long clicks_val)
    : imps(imps_val), clicks(clicks_val)
  {}

  Value&
  operator+=(const Value& rhs)
  {
    imps += rhs.imps;
    clicks += rhs.clicks;
    return *this;
  }

  unsigned long imps;
  unsigned long clicks;
};

void
Application_::generate_(
  const String::SubString& input_folder,
  const String::SubString& result_file)
{
  typedef AdServer::CampaignSvcs::RevenueDecimal RevenueDecimal;

  std::set<std::string> files;
  DirSelectAdapter dir_select_adapter(files);

  Generics::DirSelect::directory_selector(
    input_folder.str().c_str(),
    dir_select_adapter,
    "[A-Z~]*",
    Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
      Generics::DirSelect::DSF_EXCEPTION_ON_OPEN);

  Generics::GnuHashTable<Key, Value> res;
  unsigned long sum_imps = 0;
  unsigned long sum_clicks = 0;
  unsigned long records_reached_1000_imps = 0;
  unsigned long records_reached_10000_imps = 0;

  for(auto file_it = files.rbegin(); file_it != files.rend(); ++file_it)
  {
    // read file
    std::cout << "Processing " << *file_it << ": " << res.size() << " records"
      ", imps = " << sum_imps <<
      ", clicks = " << sum_clicks <<
      ", 1000 imp records = " << records_reached_1000_imps <<
      ", 10k imp records = " << records_reached_10000_imps <<      
      std::endl;

    std::ifstream istr(file_it->c_str());
    while(!istr.eof())
    {
      std::string line;
      std::getline(istr, line);
      String::StringManip::Splitter<String::AsciiStringManip::SepTab> splitter(line);
      String::SubString tag_id_str;
      String::SubString domain;
      String::SubString imps_str;
      String::SubString clicks_str;
      String::SubString nop;
      splitter.get_token(tag_id_str);
      splitter.get_token(domain);
      splitter.get_token(nop);
      splitter.get_token(nop);
      splitter.get_token(imps_str);
      splitter.get_token(clicks_str);

      unsigned long tag_id{0};
      String::StringManip::str_to_int(tag_id_str, tag_id);
      unsigned long imps{0};
      String::StringManip::str_to_int(imps_str, imps);
      unsigned long clicks{0};
      String::StringManip::str_to_int(clicks_str, clicks);

      sum_imps += imps;
      sum_clicks += clicks;
      if(domain == EMPTY_DOMAIN_MARKER)
      {
        domain = String::SubString();
      }

      Value& res_value = res[Key(tag_id, domain.str())];
      if(res_value.imps > 100000)
      {
        // stop collecting for old data
      }
      else
      {
        res_value += Value(imps, clicks);

        if(res_value.imps - imps < 1000 && res_value.imps >= 1000)
        {
          ++records_reached_1000_imps;
        }

        if(res_value.imps - imps < 10000 && res_value.imps >= 10000)
        {
          ++records_reached_10000_imps;
        }
      }
    }
  }
  
  // fetch table
  std::unordered_map<unsigned long, Value> tag_res;
  unsigned long gt1000_records = 0;
  unsigned long gt10000_records = 0;

  std::ofstream result_ostr(result_file.str().c_str());

  const RevenueDecimal C = RevenueDecimal("0.5");
  const RevenueDecimal ONE = RevenueDecimal(false, 1, 0);

  for(auto it = res.begin(); it != res.end(); ++it)
  {
    AdServer::Commons::Optional<RevenueDecimal> ctr;

    if(it->second.imps >= TRUST_IMPS)
    {
      ++gt10000_records;

      ctr = RevenueDecimal::div(
        RevenueDecimal(false, it->second.clicks, 0),
        RevenueDecimal(false, it->second.imps, 0));
    }
    else if(it->second.imps > 0)
    {
      // generate without single click
      ++gt1000_records;

      RevenueDecimal CORR_COEF = C + RevenueDecimal::div(
        RevenueDecimal::mul(ONE - C, RevenueDecimal(false, it->second.imps, 0), Generics::DMR_CEIL),
        RevenueDecimal(false, TRUST_IMPS, 0));

      RevenueDecimal base_ctr = RevenueDecimal::div(
        RevenueDecimal(false, it->second.clicks, 0),
        RevenueDecimal(false, it->second.imps, 0));

      ctr = RevenueDecimal::mul(base_ctr, CORR_COEF, Generics::DMR_CEIL);
    }

    if(it->second.imps > 0 && it->second.imps < 100)
    {
      //++no_data_records;
      tag_res[it->first.tag_id] += it->second;
    }

    /*
    else
    {
      ++no_data_records;
      tag_res[it->first.tag_id] += it->second;

      ctr = RevenueDecimal::div(
        RevenueDecimal(false, it->second.clicks > 0 ? 2 * it->second.clicks - 1 : 0, 0),
        RevenueDecimal(false, 2 * it->second.imps, 0));
    }
    */

    if(ctr.has_value())
    {
      result_ostr << it->first.tag_id << "," << (
        !it->first.domain.empty() ? it->first.domain : EMPTY_DOMAIN_MARKER) <<
        "," << ctr->str() << std::endl;
    }
  }

  // save tag level ctr's
  const RevenueDecimal COEF("0.1");
  unsigned long tag_sum_imps = 0;
  unsigned long tag_sum_clicks = 0;

  for(auto it = tag_res.begin(); it != tag_res.end(); ++it)
  {
    if(it->first > 0 && it->second.imps > 0)
    {
      RevenueDecimal ctr = RevenueDecimal::mul(
        RevenueDecimal::div(
          RevenueDecimal(false, it->second.imps >= 1000 ? it->second.clicks : (
            it->second.clicks > 0 ? it->second.clicks - 1 : 0), 0),
          RevenueDecimal(false, it->second.imps, 0)),
        COEF,
        Generics::DMR_FLOOR);

      tag_sum_imps += it->second.imps;
      tag_sum_clicks += it->second.clicks;
      result_ostr << it->first << ",?," << ctr.str() << std::endl;
    }
  }

  // save global ctr
  std::cout << "Global CTR: "
    " imps = " << tag_sum_imps <<
    ", clicks = " << tag_sum_clicks <<      
    std::endl;

  const RevenueDecimal DEFAULT_CTR = RevenueDecimal::mul(
    RevenueDecimal::div(
      RevenueDecimal(false, tag_sum_clicks, 0),
      RevenueDecimal(false, tag_sum_imps, 0)),
    RevenueDecimal("0.5"),
    Generics::DMR_FLOOR);

  result_ostr << "0,?," << DEFAULT_CTR.str() << std::endl;
  result_ostr.close();
}

int main(int argc, char** argv)
{
  try
  {
    return Application::instance().main(argc, argv);
  }
  catch(const Application_::InvalidArgument& ex)
  {
    std::cerr << "Caught InvalidArgument: " << ex.what() << std::endl;
    return -1;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}


