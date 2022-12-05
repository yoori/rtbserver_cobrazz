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

#include <Commons/ErrorHandler.hpp>
#include <Commons/CsvReader.hpp>
#include <Commons/Algs.hpp>
#include <Commons/FileManip.hpp>

#include <DTree/Gain.hpp>

#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "SVMUtil [correlate]\n";
}

/*
class SizeHolder: public ReferenceCounting::AtomicImpl
{
  SizeHolder()
    : size(0)
  {}

  unsigned long size;
};

typedef ReferenceCounting::SmartPtr<SizeHolder>
  SizeHolder_var;

class FeatureRowIndexes:
  public ReferenceCounting::AtomicImpl,
  public std::vector<unsigned long>
{
public:
  FeatureRowIndexes(
    SizeHolder* size_holder,
    std::vector<unsigned long>&& indexes)
    : size_holder_(ReferenceCounting::add_ref(size_holder))
  {
    size_holder_->size += indexes.size();
    this->swap(indexes);
  }

protected:
  virtual
  ~FeatureRowIndexes() noexcept
  {
    size_holder_->size -= this->size();
  }
};

typedef ReferenceCounting::SmartPtr<FeatureRowIndexes>
  FeatureRowIndexes_var;

class FeatureRowIndexesProvider
{
public:
  FeatureRowsProvider(
    const char* file_path,
    unsigned long max_size)
    : file_path_(file_path),
      max_size_(max_size),
      size_holder_(new SizeHolder())
  {}

  FeatureRowIndexes_var
  get(unsigned long feature_id)
  {
    auto feature_it = features_.find(feature_id);
    if(feature_it == features_.end())
    {
      // do load
      load_(feature_id);
    }

    return feature_it->second;
  }

protected:
  typedef std::map<unsigned long, FeatureRowIndexes_var>
    FeatureRowIndexesMap;
  typedef std::vector<FeatureRowIndexes_var>
    UseArray;

protected:
  void
  load_(unsigned long feature_id)
  {
    std::ifstream in(file_path_.c_str());

    FeatureArray features;
    features.reserve(10240);

    while(!in.eof())
    {
      BoolLabel label;
      Row_var new_row = SVMImpl::load_line_(in, label, features);

      if(new_row)
      {
        for(auto feature_it = new_row->features.begin();
          feature_it != new_row->features.end(); ++feature_it)
        {
        }
      }
    }
  }

protected:
  const std::string file_path_;
  const unsigned long max_size_;

  SizeHolder_var size_holder_;
  FeatureRowIndexesMap features_;
  UseArray use_order_;
};
*/

struct Counter
{
  Counter()
    : count(0)
  {}

  unsigned long count;
};

// Application
Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::load_feature_counters_(
  CountFeatureSet& feature_counters,
  const char* file_path)
{
  std::ifstream in(file_path);

  std::map<unsigned long, Counter> feature_to_counter;
  FeatureArray features;
  features.reserve(10240);

  while(!in.eof())
  {
    BoolLabel label;
    Row_var new_row = SVMImpl::load_line_(in, label, features);

    for(auto feature_it = new_row->features.begin();
      feature_it != new_row->features.end(); ++feature_it)
    {
      ++feature_to_counter[feature_it->first].count;
    }
  }

  for(auto f_it = feature_to_counter.begin();
    f_it != feature_to_counter.end(); ++f_it)
  {
    feature_counters.insert(CountFeatureKey(f_it->second.count, f_it->first));
  }
}

/*
void
Application_::correlate_2_(
  FeatureIdSet& skip_features,
  const CountFeatureSet& feature_counters,
  const char* file_path,
  const FeatureIdSet& ignore_features,
  unsigned long max_features,
  double max_corr_coef,
  unsigned long min_occur_coef)
{
  unsigned long feature_i = 0;

  for(auto feature_count_it = feature_counters.begin();
    feature_count_it != feature_counters.end();
    ++feature_count_it, ++feature_i)
  {
    if(skip_features.find(feature_count_it->first) == skip_features.end())
    {
      // process feature
      // find upper X: left.count / X.count >= max_corr_coef
      //   X.count <= left.count / max_corr_coef
      auto begin_check_it = feature_count_it;
      ++begin_check_it;

      auto end_check_it = feature_counters.lower_bound(
        FeatureCountKey(
          static_cast<unsigned long>(
            std::ceil(static_cast<double>(feature_count_it->second.count) / max_corr_coef)),
          0));

      if(end_check_it == feature_counters.end())
      {
        --end_check_it;
      }

      while(end_check_it != feature_count_it &&
        static_cast<double>(feature_count_it->second.count / end_check_it->second.count) <
          max_corr_coef)
      {
        --end_check_it;
      }

      ++end_check_it;

      // check features [begin_check_it, end_check_it)
      unsigned long need_size = 0;
      for(auto check_it = begin_check_it; check_it != end_check_it; ++check_it)
      {
        if(!row_provider->loaded(check_it->first))
        {
          need_size += check_it->second.count;
          if(need_size > max_size)
          {
            end_check_it = ;
          }
        }
      }
    }
  }
}
*/

void
Application_::correlate_(
  FeatureIdSet& checked_features,
  FeatureIdSet& skip_features,
  bool& some_feature_unloaded,
  const char* file_path,
  const FeatureIdSet& untouch_features,
  const FeatureIdSet& ignore_features,
  unsigned long max_features,
  double max_corr_coef,
  unsigned long min_occur_coef)
{
  some_feature_unloaded = false;
  checked_features.clear();

  SVMImpl_var svm(new SVMImpl());

  // load manually with applied opt_max_features,
  // fill checked_features
  std::ifstream in(file_path);

  unsigned long line_i = 0;
  FeatureArray features;
  features.reserve(10240);

  /*
  std::cerr << "*************" << std::endl;
  std::cerr << "  ignore: ";
  Algs::print(std::cerr, ignore_features.begin(), ignore_features.end());
  std::cerr << std::endl;
  std::cerr << "  skip: ";
  Algs::print(std::cerr, skip_features.begin(), skip_features.end());
  std::cerr << std::endl;
  */

  while(!in.eof())
  {
    BoolLabel label;
    Row_var new_row = SVMImpl::load_line_(in, label, features);

    if(new_row)
    {
      FeatureArray filtered_features;

      for(auto feature_it = new_row->features.begin();
        feature_it != new_row->features.end(); ++feature_it)
      {
        if(ignore_features.find(feature_it->first) == ignore_features.end() &&
          skip_features.find(feature_it->first) == skip_features.end())
        {
          if(checked_features.find(feature_it->first) == checked_features.end())
          {
            if(max_features == 0 || checked_features.size() < max_features)
            {
              checked_features.insert(feature_it->first);
              filtered_features.push_back(*feature_it);
            }
            else
            {
              //std::cerr << "some_feature_unloaded by #" << feature_it->first << std::endl;
              some_feature_unloaded = true;
            }
          }
          else
          {
            filtered_features.push_back(*feature_it);
          }
        }
      }

      new_row->features.swap(filtered_features);

      svm->add_row(new_row, label);
    }

    ++line_i;

    if(line_i % 100000 == 0)
    {
      std::cerr << "loaded " << line_i << " lines" << std::endl;
    }
  }

  std::cerr << "loading finished (" << line_i << " lines)" << std::endl;

  svm->sort_();

  std::cerr << "rows sorted" << std::endl;

  // fill skip_features
  TreeLearner<BoolLabel>::FeatureIdArray all_features;
  TreeLearner<BoolLabel>::FeatureRowsMap feature_rows;

  TreeLearner<BoolLabel>::fill_feature_rows(
    all_features,
    feature_rows,
    *svm);

  // correlate
  std::cerr << "to correlate" << std::endl;

  unsigned long feature_i = 0;

  for(auto feature_rows_it = feature_rows.begin();
    feature_rows_it != feature_rows.end();
    ++feature_rows_it, ++feature_i)
  {
    if(skip_features.find(feature_rows_it->first) == skip_features.end())
    {
      if(feature_rows_it->second->size() < min_occur_coef)
      {
        skip_features.insert(feature_rows_it->first);
      }
      else
      {
        const bool cur_feature_untouchable = (
          untouch_features.find(feature_rows_it->first) != untouch_features.end());

        auto sub_feature_rows_it = feature_rows_it;
        ++sub_feature_rows_it;
        for(; sub_feature_rows_it != feature_rows.end(); ++sub_feature_rows_it)
        {
          const bool cur_sub_feature_untouchable = (
            untouch_features.find(sub_feature_rows_it->first) != untouch_features.end());

          if(!cur_feature_untouchable || !cur_sub_feature_untouchable)
          {
            if(skip_features.find(sub_feature_rows_it->first) == skip_features.end())
            {
              // correlate features
              unsigned long left_size = feature_rows_it->second->size();
              unsigned long right_size = sub_feature_rows_it->second->size();
              unsigned long optima_cross = std::min(
                feature_rows_it->second->size(),
                sub_feature_rows_it->second->size());

              if(static_cast<double>(optima_cross) / (left_size + right_size - optima_cross) + 0.000001 >=
                   max_corr_coef)
              {
                unsigned long cross_count;
                unsigned long left_diff_count;
                unsigned long right_diff_count;

                SVM<BoolLabel>::cross_count(
                  cross_count,
                  left_diff_count,
                  right_diff_count,
                  feature_rows_it->second,
                  sub_feature_rows_it->second);

                if(cross_count + left_diff_count + right_diff_count > 0)
                {
                  if(static_cast<double>(cross_count) / (
                       cross_count + left_diff_count + right_diff_count) + 0.000001 >=
                     max_corr_coef)
                  {
                    if(!cur_sub_feature_untouchable)
                    {
                      skip_features.insert(sub_feature_rows_it->first);
                    }
                    else
                    {
                      skip_features.insert(feature_rows_it->first);
                      break; // leave sub loop
                    }
                  }
                }
              }
            }
          } // both untouchable
        }
      }
    }

    if(feature_i % 100 == 0)
    {
      std::cerr << "processed " << feature_i << "/" <<
        feature_rows.size() << ", skipped " << skip_features.size() <<
        std::endl;
    }
  }

  std::cerr << "processed " << feature_i << "/" <<
    feature_rows.size() << ", skipped " << skip_features.size() <<
    std::endl;
}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::Option<double> opt_max_corr_coef(0.99);
  Generics::AppUtils::Option<double> opt_min_occur_coef(0);
  Generics::AppUtils::Option<unsigned long> opt_max_features(0);
  Generics::AppUtils::StringOption opt_untouch_features;

  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);
  args.add(
    Generics::AppUtils::equal_name("max-corr"),
    opt_max_corr_coef);
  args.add(
    Generics::AppUtils::equal_name("min-occur"),
    opt_min_occur_coef);
  args.add(
    Generics::AppUtils::equal_name("max-features"),
    opt_max_features);
  args.add(
    Generics::AppUtils::equal_name("untouch-features"),
    opt_untouch_features);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  FeatureIdSet untouch_features;

  {
    String::StringManip::Splitter<
      String::AsciiStringManip::SepSpace> tokenizer(*opt_untouch_features);
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

        untouch_features.insert(feature_id);
      }
    }
  }
  
  auto command_it = commands.begin();
  std::string command = *command_it;
  ++command_it;
  
  if(command == "correlate")
  {
    if(command_it == commands.end())
    {
      std::cerr << "source file not defined" << std::endl;
      return;
    }

    const std::string in_file_path = *command_it;

    FeatureIdSet prev_skip_features;
    FeatureIdSet skip_features;
    unsigned long correlate_i = 0;
    bool fully_loaded = false;

    do
    {
      prev_skip_features = skip_features;

      FeatureIdSet loaded_features;
      FeatureIdSet checked_features;
      bool some_feature_unloaded = false;

      unsigned long sub_correlate_i = 0;

      do
      {
        correlate_(
          checked_features,
          skip_features,
          some_feature_unloaded,
          in_file_path.c_str(),
          untouch_features,
          loaded_features,
          *opt_max_features,
          *opt_max_corr_coef,
          *opt_min_occur_coef);

        std::copy(
          checked_features.begin(),
          checked_features.end(),
          std::inserter(loaded_features, loaded_features.begin()));

        std::cerr << "correlate #" << correlate_i << "." <<
          sub_correlate_i << ": " <<
          skip_features.size() << " skipped, " <<
          checked_features.size() << " checked, " <<
          loaded_features.size() << " loaded, " <<
          "some_feature_unloaded = " << some_feature_unloaded <<
          std::endl;

        ++sub_correlate_i;
      }
      while(some_feature_unloaded);

      if(sub_correlate_i == 1)
      {
        // fully loaded at first iteration
        fully_loaded = true;
      }

      ++correlate_i;
    }
    while(prev_skip_features.size() != skip_features.size() && !fully_loaded);

    {
      // final load & save with applied skip features
      std::ifstream in(in_file_path.c_str());

      FeatureArray features;
      features.reserve(10240);

      while(!in.eof())
      {
        BoolLabel label;
        Row_var new_row = SVMImpl::load_line_(in, label, features);

        if(new_row)
        {
          FeatureArray filtered_features;

          for(auto feature_it = new_row->features.begin();
            feature_it != new_row->features.end(); ++feature_it)
          {
            if(skip_features.find(feature_it->first) == skip_features.end())
            {
              filtered_features.push_back(*feature_it);
            }
          }

          new_row->features.swap(filtered_features);

          SVMImpl::save_line(std::cout, new_row, label);
        }

        /*
        ++line_i;

        if(line_i % 100000 == 0)
        {
          std::cerr << "saved " << line_i << " lines" << std::endl;
        }
        */
      }

      std::cerr << "saving finished" << std::endl;
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


