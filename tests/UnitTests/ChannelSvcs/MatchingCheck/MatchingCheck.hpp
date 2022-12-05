#ifndef MATCHING_CHECK_TEST_HPP_
#define MATCHING_CHECK_TEST_HPP_

#include<tests/UnitTests/ChannelSvcs/Commons/ChannelServerTestCommons.hpp>

namespace AdServer
{
namespace UnitTests
{
  class MatchingCheckTest: public TestTemplate
  {
  public:
    int run() noexcept;
  private:
    typedef std::map<unsigned long, std::vector<std::string> > SaveType;

    void prepare_data_(SaveType& atoms_save)
      /*throw(eh::Exception)*/;

    void make_check_(const SaveType& atoms)
      /*throw(eh::Exception)*/;

    void match_i_(const String::SubString& in) /*throw(eh::Exception)*/;

    void reset_staitstic_() /*throw(eh::Exception)*/;
  };
}
}

#endif

