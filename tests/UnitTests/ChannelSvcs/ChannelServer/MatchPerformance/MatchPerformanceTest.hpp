#ifndef MATCH_PERFORMANCE_TEST_DEF
#define MATCH_PERFORMANCE_TEST_DEF

#include <ChannelSvcs/ChannelServer/ChannelContainer.hpp>

namespace AdServer
{
  namespace UnitTests
  {

    class MatchPerformanceTest: public TestTemplate
    {
    public:
      MatchPerformanceTest() noexcept;
      int run(int argc, char* argv[]) noexcept;
    private:

      void prepare_data_(
        ChannelSvcs::UpdateContainer& cont,
        ChannelSvcs::ChannelIdToMatchInfo* info
        ) /*throw(eh::Exception)*/;

      int execute_() /*throw(eh::Exception)*/;

      void generate_word(std::string& word) /*throw(eh::Exception)*/;

      void generate_url_(std::string& word) /*throw(eh::Exception)*/;

      void prepare_match_data_(
        unsigned long first_id,
        unsigned long count,
        ChannelSvcs::ChannelContainerBase::ChannelMap& buffer)
        /*throw(eh::Exception)*/;

    };

  }
}

#endif

