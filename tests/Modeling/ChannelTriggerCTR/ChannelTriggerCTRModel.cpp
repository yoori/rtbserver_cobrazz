#include <math.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <assert.h>

#include <Generics/AppUtils.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>
#include <Commons/Algs.hpp>

/** Use examples:
 *    -u 100 -r 100 -t 5 --clicks='0:0.5,2:0.5,4:1' -g2 -v
 *      100 users,
 *      100 requests per user,
 *      5 triggers in channel
 *      2 triggers in group (1 fake trigger)
 *      click only triggers #0,2 (with probability 0.5), #4 - always
 */
enum GenerateType
{
  GENERATE_GROUP_PER_USER = 0,
  GENERATE_GROUP_PER_REQUEST = 1,
};

enum PrintType
{
  PT_SIMPLE = 0,
  PT_EXTEND
};

typedef std::map<unsigned long, double> ClickTriggerMap;

class User
{
public:
  void generate_group(
    std::set<unsigned long>& match_triggers,
    std::set<unsigned long>& negative_triggers,
    GenerateType generate_type,
    int& real_match_trigger,
    bool& do_click,
    int triggers_in_channel,
    int trigger_group_size,
    int negative_trigger_group_size,
    const ClickTriggerMap& click_triggers)
    const
  {
    if(triggers_in_channel == negative_trigger_group_size)
    {
      for(int i = 0; i < triggers_in_channel; ++i)
      {
        negative_triggers.insert(i);
      }
    }
    else
    {
      std::vector<unsigned long> rand_set;
      rand_set.reserve(negative_trigger_group_size);

      for(int i = 0; i < negative_trigger_group_size; ++i)
      {
        unsigned long new_val = ::rand() % (triggers_in_channel - i);
        unsigned long add_ind = 0;
        std::vector<unsigned long>::iterator vit = rand_set.begin();
        for(; vit != rand_set.end() && new_val + add_ind >= *vit; ++vit)
        {
          ++add_ind;
        }

        rand_set.insert(vit, new_val + add_ind);
      }

      assert(rand_set.size() == static_cast<unsigned long>(negative_trigger_group_size));
      std::copy(
        rand_set.begin(),
        rand_set.end(),
        std::inserter(negative_triggers, negative_triggers.begin()));
      assert(*negative_triggers.rbegin() < static_cast<unsigned long>(triggers_in_channel));
    }

    if(generate_type == GENERATE_GROUP_PER_REQUEST)
    {
      real_match_trigger = ::rand() % triggers_in_channel;

      std::vector<unsigned long> rand_set;
      rand_set.reserve(trigger_group_size);
      rand_set.push_back(real_match_trigger);

      for(int i = 1; i < trigger_group_size; ++i)
      {
        unsigned long new_val = ::rand() % (triggers_in_channel - i);
        unsigned long add_ind = 0;
        std::vector<unsigned long>::iterator vit = rand_set.begin();
        for(; vit != rand_set.end() && new_val + add_ind >= *vit; ++vit)
        {
          ++add_ind;
        }

        rand_set.insert(vit, new_val + add_ind);
      }

      assert(rand_set.size() == static_cast<unsigned long>(trigger_group_size));
      std::copy(
        rand_set.begin(),
        rand_set.end(),
        std::inserter(match_triggers, match_triggers.begin()));
    }
    // group per user
    else
    {
      std::vector<int> arr;

      if(triggers_group.empty())
      {
        arr.resize(triggers_in_channel, 0);
        for(int i = 0; i < triggers_in_channel; ++i)
        {
          arr[i] = i < trigger_group_size ? (i == trigger_group_size - 1 ? 2 : 1) : 0;
        }

        std::random_shuffle(arr.begin(), arr.end());
        triggers_group = arr;
      }
      else
      {
        arr = triggers_group;
      }

      real_match_trigger = 0xFFFFFFFF;
      for(int j = 0; j < triggers_in_channel; ++j)
      {
        if(arr[j] > 1)
        {
          real_match_trigger = j;
        }
      }

      for(int j = 0; j < triggers_in_channel; ++j)
      {
        if(arr[j])
        {
          match_triggers.insert(j);
        }
      }
    }

    do_click = false;

    ClickTriggerMap::const_iterator cit = click_triggers.find(real_match_trigger);
    if(cit != click_triggers.end())
    {
      if(1.0 * (::rand() % 10000) / 10000 < cit->second)
      {
        do_click = true;
      }
    }
  }

private:
  mutable std::vector<int> triggers_group;
};

void run_case(
  std::ostream& out,
  double& max_ctr_diff,
  GenerateType generate_type,
  int users_count,
  int request_per_user,
  int trigger_group_size,
  int negative_trigger_group_size,
  int triggers_in_channel,
  const ClickTriggerMap& click_triggers,
  bool print)
{
  typedef std::list<User> UserList;

  max_ctr_diff = 0;

  UserList users;

  for(int i = 0; i < users_count; ++i)
  {
    users.push_back(User());
  }

  std::vector<int> imps(triggers_in_channel, 0);
  std::vector<int> clicks(triggers_in_channel, 0); 
  std::vector<int> real_imps(triggers_in_channel, 0);
  std::vector<int> real_clicks(triggers_in_channel, 0);
  std::vector<int> excess_imps(triggers_in_channel, 0);
  std::vector<int> excess_clicks(triggers_in_channel, 0);  
  int all_imps = 0;
  int all_clicks = 0;

  for(UserList::const_iterator uit = users.begin();
      uit != users.end(); ++uit)
  {
    for(int i = 0; i < request_per_user; ++i)
    {
      std::set<unsigned long> match_triggers;
      std::set<unsigned long> negative_triggers;
      int real_match_trigger;
      bool do_click;

      uit->generate_group(
        match_triggers,
        negative_triggers,
        generate_type,
        real_match_trigger,
        do_click,
        triggers_in_channel,
        trigger_group_size,
        negative_trigger_group_size,
        click_triggers);

      for(std::set<unsigned long>::const_iterator tit =
            match_triggers.begin();
          tit != match_triggers.end(); ++tit)
      {
        imps[*tit] += 1;
        clicks[*tit] += do_click ? 1 : 0;
      }

      /*
      std::cout << "T: ";
      Algs::print(std::cout, negative_triggers.begin(), negative_triggers.end());
      std::cout << std::endl;
      */

      for(std::set<unsigned long>::const_iterator tit =
            negative_triggers.begin();
          tit != negative_triggers.end(); ++tit)
      {
        excess_imps[*tit] += 1;
        excess_clicks[*tit] += do_click ? 1 : 0;
      }

      real_imps[real_match_trigger] += 1;
      all_imps += trigger_group_size;

      if(do_click)
      {
        real_clicks[real_match_trigger] += 1;
        all_clicks += trigger_group_size;
      }
    }
  }

  if(print)
  {
    out << "all logged (c=" << all_clicks << ",i=" << all_imps <<
      ")" << std::endl;
  }
  
  std::vector<int> approximated_imps(triggers_in_channel, 0);
  std::vector<int> approximated_clicks(triggers_in_channel, 0);

  for(int i = 0; i < triggers_in_channel; ++i)
  {
    // imps * (TC - 1) / (TC - TG) - : x TG
    // eImps * TC * (TG - 1) / (NG * (TC - TG)) : x NG
    approximated_clicks[i] = static_cast<int>(
      1.0 * clicks[i] * (triggers_in_channel - 1) /
        (triggers_in_channel - trigger_group_size) -
      1.0 * excess_clicks[i] * triggers_in_channel * (trigger_group_size - 1) /
        (negative_trigger_group_size * (triggers_in_channel - trigger_group_size)));
    approximated_imps[i] = static_cast<int>(
      1.0 * imps[i] * (triggers_in_channel - 1) /
        (triggers_in_channel - trigger_group_size) -
      1.0 * excess_imps[i] * triggers_in_channel * (trigger_group_size - 1) /
        (negative_trigger_group_size * (triggers_in_channel - trigger_group_size)));

    double real_ctr = (1.0 * real_clicks[i] / real_imps[i]);
    double reconstructed_ctr = (
      approximated_imps[i] != 0 ?
      std::max(0., std::min(1., 1. * approximated_clicks[i] / approximated_imps[i])) :
      0.0);

    max_ctr_diff = std::max(max_ctr_diff, ::fabs(reconstructed_ctr - real_ctr));
  }

  if(print)
  {
    for(int i = 0; i < triggers_in_channel; ++i)
    {
      out << i << ": stat (c=" << clicks[i] << ",i=" << imps[i] << ")" <<
        ", excess (c=" << excess_clicks[i] << ",i=" << excess_imps[i] << ")" <<
        ", expected (c=" << real_clicks[i] << ",i=" << real_imps[i] << ")" <<
        ", approximated (c=" << approximated_clicks[i] << ",i=" << approximated_imps[i] << ")" <<
        ", diff (c=" << (1.0 * ::abs(real_clicks[i] - approximated_clicks[i]) / all_clicks) <<
        ",i=" << (1.0 * ::abs(real_imps[i] - approximated_imps[i]) / all_imps) << ")";
      out << std::endl;
    }

    for(int i = 0; i < triggers_in_channel; ++i)
    {
      double reconstructed_ctr = (
        approximated_imps[i] != 0 ?
        std::max(0., std::min(1., 1. * approximated_clicks[i] / approximated_imps[i])) :
        0.0);

      out << i << ": real ctr = " << (1.0 * real_clicks[i] / real_imps[i]) <<
        ", reconstructed ctr = " << reconstructed_ctr << std::endl;
    }
  }
}

void run_testing_loop(
  int run_num,
  GenerateType generate_type,
  int users_count,
  int request_per_user,
  int trigger_group_size,
  int negative_trigger_group_size,
  int triggers_in_channel,
  const ClickTriggerMap& click_triggers,
  PrintType print_type)
{
  /*
   * RECONSTRUCT: N - number of triggers in channel
   * (IMP - EXCESS / (N - 1)) / (1 - EXCESS / (N - 1))
   */
  std::cout << "Case: group generation type = '" <<
    (generate_type == GENERATE_GROUP_PER_USER ? 'U' : 'R') <<
    "', users = " << users_count <<
    ", requests = " <<
    (users_count * request_per_user) <<
    ", click_triggers =";
  for(ClickTriggerMap::const_iterator cit = click_triggers.begin();
      cit != click_triggers.end(); ++cit)
  {
    std::cout << " " << cit->first << ":" << cit->second;
  }
  std::cout << std::endl;

  ::srand(time(0));
  double max_ctr_diff = 0;
  double min_ctr_diff = 2*triggers_in_channel;
  double sum_ctr_diff = 0;
  std::string best_res_out;
  std::string badless_res_out;

  for(int run_i = 0; run_i < run_num; ++run_i)
  {
    std::ostringstream out;
    double local_max_ctr_diff;
    run_case(
      out,
      local_max_ctr_diff,
      generate_type,
      users_count,
      request_per_user,
      trigger_group_size,
      negative_trigger_group_size,
      triggers_in_channel,
      click_triggers,
      true);

    if(local_max_ctr_diff > max_ctr_diff)
    {
      badless_res_out = out.str();
    }
    if(local_max_ctr_diff < min_ctr_diff)
    {
      best_res_out = out.str();
    }

    max_ctr_diff = std::max(max_ctr_diff, local_max_ctr_diff);
    min_ctr_diff = std::min(min_ctr_diff, local_max_ctr_diff);
    sum_ctr_diff += local_max_ctr_diff;
  }

  if(print_type == PT_EXTEND)
  {
    std::cout << "Best result:" << std::endl << best_res_out;
    std::cout << "Worst result:" << std::endl << badless_res_out;
  }
  std::cout << "Max diff = " << max_ctr_diff <<
    ", Min diff = " << min_ctr_diff <<
    ", Avg diff = " << (sum_ctr_diff / run_num) << std::endl;
}

int main(int argc, char** argv)
{
  using namespace Generics::AppUtils;
  Args args;
  Option<unsigned long> opt_run_num(100);
  Option<unsigned long> opt_users_count(1000);
  Option<unsigned long> opt_req_per_user(1);
  Option<unsigned long> opt_triggers_group_size(8);
  Option<unsigned long> opt_negative_triggers_group_size(8);
  Option<unsigned long> opt_triggers_in_channel(16);
  Option<char> opt_generate_strategy('R');
  Option<std::string> opt_click_triggers;
  CheckOption opt_verbose;

  args.add(equal_name("run") || short_name("c"), opt_run_num);
  args.add(equal_name("users") || short_name("u"), opt_users_count);
  args.add(equal_name("req") || short_name("r"), opt_req_per_user);
  args.add(equal_name("group") || short_name("g"), opt_triggers_group_size);
  args.add(equal_name("ngroup") || short_name("ng"), opt_negative_triggers_group_size);
  args.add(equal_name("triggers") || short_name("t"), opt_triggers_in_channel);
  args.add(equal_name("gen") || short_name("s"), opt_generate_strategy);
  args.add(equal_name("clicks") || short_name("c"), opt_click_triggers);
  args.add(short_name("v"), opt_verbose);

  args.parse(argc - 1, argv + 1);

  PrintType print_type = PT_SIMPLE;

  if(opt_verbose.enabled())
  {
    print_type = PT_EXTEND;
  }

  unsigned long negative_triggers_group_size =
    std::min(*opt_triggers_in_channel, *opt_negative_triggers_group_size);
  std::cout << "negative_triggers_group_size = " << negative_triggers_group_size << std::endl;

  ClickTriggerMap click_triggers;

  if(!opt_click_triggers.installed())
  {
    click_triggers.insert(std::make_pair(*opt_triggers_in_channel / 4, 1.0));
    click_triggers.insert(std::make_pair(*opt_triggers_in_channel * 3 / 4, 1.0));
  }
  else
  {
    String::StringManip::SplitComma tokenizer(*opt_click_triggers);

    String::SubString tr_str;
    while (tokenizer.get_token(tr_str))
    {
      String::StringManip::trim(tr_str);
      String::SubString::SizeType pos = tr_str.find_last_of(':');
      String::SubString tr_i = tr_str;
      double ctr = 1;
      if (pos != String::SubString::NPOS)
      {
        tr_i = tr_str.substr(0, pos);
        Stream::Parser ctr_istr(tr_str.substr(pos + 1));
        ctr_istr >> ctr;
      }

      Stream::Parser tri_istr(tr_i);
      unsigned long i;
      tri_istr >> i;
      click_triggers.insert(std::make_pair(i, ctr));
    }
  }

  if(*opt_generate_strategy == 'A' || *opt_generate_strategy == 'U')
  {
    run_testing_loop(
      *opt_run_num,
      GENERATE_GROUP_PER_USER,
      *opt_users_count,
      *opt_req_per_user,
      *opt_triggers_group_size,
      negative_triggers_group_size,
      *opt_triggers_in_channel,
      click_triggers,
      print_type);
  }

  if(*opt_generate_strategy == 'A' || *opt_generate_strategy == 'R')
  {
    run_testing_loop(
      *opt_run_num,
      GENERATE_GROUP_PER_REQUEST,
      *opt_users_count,
      *opt_req_per_user,
      *opt_triggers_group_size,
      negative_triggers_group_size,
      *opt_triggers_in_channel,
      click_triggers,
      print_type);
  }

  return 0;
}
