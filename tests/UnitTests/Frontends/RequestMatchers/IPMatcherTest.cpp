#include <iostream>

#include <Frontends/FrontendCommons/RequestMatchers.hpp>

using namespace FrontendCommons;

struct Rule
{
  const char* ip_masks;
  const char* cohorts;
  unsigned long colo_id;
};

struct MatchCase
{
  const char* ip;
  const char* cohort;
  bool matched;
  unsigned long colo_id;
};

const Rule DEFAULT_MASKS[] = {
  { "0.0.0.0/0", "", 1 },
  { "0.0.0.0/0", "X", 2 },
};

const MatchCase DEFAULT_CASES[] = {
  { "1.1.1.78", "X", true, 2 },
  { "1.1.1.78", "", true, 1 },
};

const Rule MASKS[] = {
  { "1.1.1.0/24", "", 1 },
  { "1.1.2.0/24", "ct-2", 2 },
  { "1.1.3.0/24\n1.1.4.0/24", "ct-3", 3 },
  { "1.1.5.0/24\n1.1.6.0/24", "ct-4\nct-5", 4 },

  { "5.10.16.0/20", "", 5 },
  { "2.216.1.2/32", "ct-111", 6 },
};

const MatchCase CASES[] = {
  { "1.1.1.78", "", true, 1 },
  { "1.1.1.78", "ct-X", true, 1 },
  { "1.1.1.78", "ct-X.ct-Y", true, 1 },
  { "1.1.1.78", "ct-Y.ct-X", true, 1 },

  { "1.1.2.78", "", false, 0 },
  { "1.1.2.78", "", false, 0 },
  { "1.1.2.78", "ct-2", true, 2 },

  { "1.1.3.78", "ct-3", true, 3 },
  { "1.1.4.78", "ct-3", true, 3 },

  { "1.1.5.78", "", false, 0 },
  { "1.1.5.78", "ct-4", true, 4 },
  { "1.1.6.78", "ct-5", true, 4 },

  { "5.10.16.1", "", true, 5 },

  { "2.216.1.2", "ct-111", true, 6 },
};

int check_cases(
  const Rule* rules,
  unsigned long rules_count,
  const MatchCase* cases,
  unsigned long cases_count)
{
  int res = 0;

  IPMatcher_var ip_matcher = new IPMatcher();

  for(unsigned long i = 0; i < rules_count; ++i)
  {
    IPMatcher::MatchResult match_result;
    match_result.colo_id = MASKS[i].colo_id;
    ip_matcher->add_rule(
      String::SubString(rules[i].ip_masks),
      String::SubString(rules[i].cohorts),
      match_result);
  }

  for(unsigned long i = 0; i < cases_count; ++i)
  {
    IPMatcher::MatchResult match_result;
    bool matched = ip_matcher->match(
      match_result,
      String::SubString(cases[i].ip),
      String::SubString(cases[i].cohort));

    bool success = (
      (matched && cases[i].matched && match_result.colo_id == cases[i].colo_id) ||
      (!matched && !cases[i].matched));

    if(!success)
    {
      std::cerr << "case #" << i << " failed: "
        "ip = " << cases[i].ip <<
        ", cohort = " << cases[i].cohort <<
        "; result matched = " << matched <<
        ", colo_id = " << match_result.colo_id << std::endl;
      res = 1;
    }
  }

  return res;
}

int main()
{
  int res = 0;

  res += check_cases(
    DEFAULT_MASKS,
    sizeof(DEFAULT_MASKS) / sizeof(DEFAULT_MASKS[0]),
    DEFAULT_CASES,
    sizeof(DEFAULT_CASES) / sizeof(DEFAULT_CASES[0]));

  res += check_cases(
    MASKS,
    sizeof(MASKS) / sizeof(MASKS[0]),
    CASES,
    sizeof(CASES) / sizeof(CASES[0]));

  return res;
}
