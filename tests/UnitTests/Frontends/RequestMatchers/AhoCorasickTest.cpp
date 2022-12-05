#include <iostream>
#include <vector>
#include <Commons/Algs.hpp>
#include <Frontends/FrontendCommons/AhoCorasick.hpp>

using namespace FrontendCommons;

typedef int Tag;
typedef std::vector<Tag> Tags;

class MatchCallback : public std::unary_function<int, void>
{
public:
  MatchCallback(Tags& matched_tags) noexcept
    : matched_tags_(matched_tags)
  {}

  bool
  operator()(const AhoCorasik<Tag>::MatchDetails& details) const noexcept
  {
    matched_tags_.push_back(details.tag);
    return false;
  }

private:
  Tags& matched_tags_;
};

template<const unsigned int ETALON_SIZE>
bool
check_match_result(
  const char* test_name,
  const Tags& matched_tags,
  const int (&etalon_tags)[ETALON_SIZE])
{
  if(matched_tags.size() != ETALON_SIZE ||
     !std::equal(matched_tags.begin(), matched_tags.end(), etalon_tags))
  {
    std::cerr << test_name << ": unexpected match result:" << std::endl << "  ";
    Algs::print(std::cerr, matched_tags.begin(), matched_tags.end());
    std::cerr << std::endl << "instead" << std::endl << "  ";
    Algs::print(std::cerr, etalon_tags, etalon_tags + ETALON_SIZE);
    std::cerr << std::endl;
    return false;
  }

  return true;
}

int
empty_do_not_fault_test() noexcept
{
  static const char* TEST = "empty_do_not_fault_test";

  Tags matched_tags;
  MatchCallback callback(matched_tags);
  AhoCorasik<Tag> matcher;

  matcher.match(String::SubString("AAAAA"), callback);
  if(!matched_tags.empty())
  {
    std::cerr << TEST << ": error, non empty result: ";
    Algs::print(std::cerr, matched_tags.begin(), matched_tags.end());
    std::cerr << std::endl;
    return 1;
  }
  std::cout << TEST << ": success" << std::endl;
  return 0;
}

int
match_test_1() noexcept
{
  static const char* TEST = "match_test_1";

  Tags matched_tags;
  MatchCallback callback(matched_tags);
  AhoCorasik<Tag> matcher(false);

  matcher.add_pattern("win", 1);
  matcher.add_pattern("winwin", 2);
  matcher.add_pattern("abbat", 3);
  matcher.add_pattern("Abba", 4);
  matcher.add_pattern("AAB", 5);

  matcher.match(String::SubString("awinwinwinwinAbbatAAABBA"), callback);

  const int ETALON_TAGS[] = { 1, 1, 2, 1, 2, 1, 2, 4, 3, 5, 4 };
  if(!check_match_result(TEST, matched_tags, ETALON_TAGS))
  {
    return 1;
  }

  std::cout << TEST << ": success" << std::endl;
  return 0;
}

int
match_test_2() noexcept
{
  // regression
  static const char* TEST = "match_test_2";

  Tags matched_tags;
  MatchCallback callback(matched_tags);
  AhoCorasik<Tag> matcher(false);

  matcher.add_pattern("abcde", 1);
  matcher.add_pattern("bcd", 2);
  matcher.add_pattern("abc", 3);
  matcher.add_pattern("cde", 4);

  matcher.match(String::SubString("ababcdeab"), callback);

  const int ETALON_TAGS[] = { 3, 2, 1, 4 };
  if(!check_match_result(TEST, matched_tags, ETALON_TAGS))
  {
    return 1;
  }

  std::cout << TEST << ": success" << std::endl;
  return 0;
}

int main() noexcept
{
  int res = empty_do_not_fault_test();
  res += match_test_1();
  res += match_test_2();
  return res;
}
