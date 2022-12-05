#ifndef TRIGGEER_FILTERING_APP_HPP
#define TRIGGEER_FILTERING_APP_HPP

#include<string>
#include<map>
#include<set>
#include<istream>
#include<fstream>
#include<ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include<ChannelSvcs/ChannelServer/ChannelContainer.hpp>

namespace AdServer
{
  class Application
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(BadParams, Exception);

    int run(int argc, char* argv[]) /*throw(BadParams)*/;

  private:

    void load_dictionary_(const char* file_name) /*throw(BadParams)*/;

    void read_triggers_(std::istream& is) /*throw(eh::Exception)*/;

    void write_result_(std::ostream& os) noexcept;

    void filter_() /*throw(Exception)*/;

    void usage_(const char* program_name) noexcept;

    static
    int compary_triggers_(
      const Language::Trigger::Trigger& trigger_view,
      const Language::Trigger::Trigger& trigger_view2)
      noexcept;

  private:
    typedef std::map<unsigned long, std::string> Triggers;

    Triggers triggers_;
    AdServer::ChannelSvcs::ChannelContainer container_;
    ChannelSvcs::StringSet dict_;
  };
}

#endif
