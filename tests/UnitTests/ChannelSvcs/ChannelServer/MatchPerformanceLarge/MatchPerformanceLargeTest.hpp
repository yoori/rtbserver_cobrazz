#ifndef MATCH_PERFORMANCE_LAST__TEST_HPP
#define MATCH_PERFORMANCE__LAST_TEST_HPP

#include <ChannelSvcs/ChannelServer/ChannelContainer.hpp>

namespace AdServer
{
  namespace UnitTests
  {

    class MatchPerformanceLargeTest: public TestTemplate
    {
    public:
      MatchPerformanceLargeTest() noexcept;
      int run(int argc, char* argv[]) noexcept;
      int help() noexcept;
    private:

      void fill_matched_words_(
        std::vector<std::string>& matched_words,
        size_t count_hard,
        size_t count_soft) noexcept;

      void prepare_data_(
        ChannelSvcs::UpdateContainer& cont,
        ChannelSvcs::ChannelIdToMatchInfo* info
        ) /*throw(eh::Exception)*/;

      int execute_() /*throw(eh::Exception)*/;

      void generate_url_(std::string& word) /*throw(eh::Exception)*/;

      void prepare_match_data_(
        unsigned long first_id,
        unsigned long count,
        ChannelSvcs::ChannelContainerBase::ChannelMap& buffer)
        /*throw(eh::Exception)*/;

      ChannelSvcs::MergeAtom& new_atom_(
        unsigned long id,
        size_t type,
        std::map<unsigned long, ChannelSvcs::MergeAtom>& atoms,
        ChannelSvcs::ChannelIdToMatchInfo* info)
        /*throw(eh::Exception)*/;

      void parse_keywords_(ChannelSvcs::MatchWords& out) noexcept;

    };

  }
}

#endif

