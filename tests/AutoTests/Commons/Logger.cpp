#include <sys/types.h>
#include <sys/stat.h>
#include <ace/OS_NS_Thread.h>
#include <fstream>
#include <Logger/StreamLogger.hpp>

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{

  std::string
  get_logger_name(
    const std::string& description)
  {
    std::string temp, result;
    for(std::string::const_iterator i =  description.begin();
      i != description.end(); ++i)
    {
      char c = String::AsciiStringManip::ALPHA_NUM.is_owned(*i)? *i: '_';
      temp.push_back(c);
    }
    String::StringManip::trim(
      temp,
      result,
      String::AsciiStringManip::CharCategory("_"));
    return result;
  }

  Logger::Logger(const char* log_name):
    Base(nullptr),
    log_streams_(),
    log_name_(log_name),
    empty_(false)
  {
    init_(AutoTest::GlobalSettings::instance().config());
  }

  Logger::~Logger() noexcept
  {
    for (LogStreamsCont::iterator it = log_streams_.begin();
         it != log_streams_.end(); ++it)
    {
      delete (*it);
    }
  }

  void
  Logger::init_(Params params)
    /*throw(eh::Exception)*/
  {
    {/////////////
      struct stat buf;
      if (stat(params.LoggerGroup().path().c_str(), &buf))
      {
        mkdir(params.LoggerGroup().path().c_str(), 0755);
      }
    }////////////
    std::string filename_base =
      params.LoggerGroup().path() + "/" + log_name_ + ".";

    ReferenceCounting::Deque<Logging::QLogger_var> loggers;

    typedef
      xsd::AdServer::Configuration::LoggerGroupType::LogFile_sequence::const_iterator
      LogFileIterator;

    LogFileIterator it = params.LoggerGroup().LogFile().begin(),
      end = params.LoggerGroup().LogFile().end();
    for (; it != end; ++it)
    {
      Logging::Logger::Severity from
        (static_cast<Logging::Logger::Severity>(it->level_from()));
      Logging::Logger::Severity to
        (static_cast<Logging::Logger::Severity>(it->level_to()));

      Logging::Logger_var stream_logger;
      if (it->extension().empty())
      {
        stream_logger = new Logging::OStream::Logger(
          Logging::OStream::Config(std::cerr, to));
      }
      else
      {
        std::string filename = filename_base + it->extension();
        std::ofstream *stream = new std::ofstream(filename.c_str());
        if (stream->bad() || stream->fail())
        {
          Stream::Error error;
            error << "BaseUnit::get_logger_for_task(): failed to create file '"
                  << filename << "'.  Check that directory exists";

          throw Exception(error);
        }
        log_streams_.push_back(stream);
        stream_logger = new Logging::OStream::Logger(
          Logging::OStream::Config(*stream, to));
      }

      loggers.push_back(Logging::FLogger_var(
        new Logging::SeveritySelectorLogger(stream_logger, from, to)));
    }
    Base::logger_ = new Logging::DistributorLogger(
      loggers.begin(), loggers.end());
  }

  void
  Logger::clear_loggers() /*throw(eh::Exception)*/
  {
    Base::logger_ = new Logging::Null::Logger;
    log_streams_.clear();
    empty_ = true;
  }

  bool
  Logger::info(const String::SubString& text)
    noexcept
  {
    return Base::log(text, Logging::Logger::INFO, log_name_.c_str());
  }

  bool
  Logger::debug(const String::SubString& text)
    noexcept
  {
    return Base::log(text, Logging::Logger::DEBUG, log_name_.c_str());
  }

  bool
  Logger::trace(const String::SubString& text)
    noexcept
  {
    return Base::log(text, Logging::Logger::TRACE, log_name_.c_str());
  }

  bool
  Logger::debug_trace(const String::SubString& text)
    noexcept
  {
    return Base::log(text, Logging::Logger::TRACE+100, log_name_.c_str());
  }

  bool
  Logger::error(const String::SubString& text)
    noexcept
  {
    return Base::log(text, Logging::Logger::ERROR, log_name_.c_str());
  }

  bool
  Logger::warning(const String::SubString& text)
    noexcept
  {
    return Base::log(text, Logging::Logger::WARNING, log_name_.c_str());
  }

  Sync::Key<Logger> Logger::key_;

  Logger&
  Logger::thlog ()
  {
    return *key_.get_data();
  }

  void
  Logger::thlog (Logger& log)
  {
    key_.set_data(&log);
  }

  // LoggerSwitcher

  LoggerSwitcher::LoggerSwitcher(
    Logger& new_logger) :
    old_logger_(Logger::thlog())
  {
    if (&new_logger != &old_logger_)
    {
      Logger::thlog(new_logger);
    }
  }

  LoggerSwitcher::~LoggerSwitcher()
  {
    if (&old_logger_ != &Logger::thlog())
    {
      Logger::thlog(old_logger_);
    }
  }

}// AutoTest
