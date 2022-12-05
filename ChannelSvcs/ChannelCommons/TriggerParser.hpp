
#ifndef CHANNEL_SERVICE_CHANNEL_PARSER_HPP_
#define CHANNEL_SERVICE_CHANNEL_PARSER_HPP_

#include <set>
#include <Logger/Logger.hpp>
#include <ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include <ChannelSvcs/ChannelCommons/Serialization.hpp>
#include <Language/SegmentorCommons/SegmentorInterface.hpp>


namespace AdServer
{
namespace ChannelSvcs
{

  namespace TriggerParser
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    class MergeContainer
    {
    public:
      virtual ~MergeContainer() noexcept{};

      virtual size_t add_trigger(MergeAtom& atom)
        /*throw(TriggerParser::Exception)*/ = 0;

      virtual bool ready() const noexcept { return true; }
    };

    class TriggerParser
    {
    public:

      static void parse_word(
        IdType channel_id,
        unsigned int channel_trigger_id,
        char type,
        const std::string& trigger,
        bool negative,
        const Language::Segmentor::SegmentorInterface* segmentor,
        SoftTriggerList& soft_words,
        size_t seq_length,
        Logging::Logger* logger)
        /*throw(Exception)*/;

      static void parse_url(
        IdType channel_id,
        unsigned int channel_trigger_id,
        const std::string& trigger,
        bool negative,
        const std::set<unsigned short>& allow_ports,
        SoftTriggerList& url_words,
        Logging::Logger* logger)
        /*throw(Exception)*/;

      static void parse_uids(
        IdType channel_id,
        const std::string& trigger,
        std::string& uids_trigger,
        Logging::Logger* logger)
        /*throw(Exception)*/;

      static size_t parse_triggers(
        unsigned int id,
        const std::string& lang,
        const TriggerList& triggers,
        const Language::Segmentor::SegmentorInterface* segmentor,
        const std::set<unsigned short>& allow_ports,
        MergeContainer* out,
        size_t seq_length,
        Logging::Logger* logger = 0)
        /*throw(Exception)*/;
        
      static const unsigned long MAX_TRIGGER_LENGTH;

      static const unsigned long MAX_URL_LENGTH;
      static const unsigned long WORSE_MULT;

    };

    class ParseTracer
    {
    public:
      ParseTracer(bool active = true) noexcept;

      void start() noexcept;
      void get_time(Generics::Time& time, Generics::Time& cpu_time) noexcept;
      void add_time(Generics::Time& time, Generics::Time& cpu_time) noexcept;

    private:
      Generics::Timer timer_;
      Generics::CPUTimer cpu_timer_;
      bool active_;
    };

  }
}// namespace ChannelSvcs
}

#endif //CHANNEL_SERVICE_CHANNEL_PARSER_HPP_

