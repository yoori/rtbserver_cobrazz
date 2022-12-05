#ifndef __AUTOTESTS_COMMONS_LOGGER_HPP
#define __AUTOTESTS_COMMONS_LOGGER_HPP

#include <vector>
#include <Sync/Key.hpp>
#include <Logger/DistributorLogger.hpp>
#include <tests/AutoTests/Commons/AutoTestsXsd.hpp>

namespace AutoTest
{
  /**
   * @brief Get(prepare) logger name from case description
   *
   * @param description
   */
  std::string
  get_logger_name(
    const std::string& description);
  
  /**
   * @class Logger
   * @brief Base logging class for AdServer AutoTests.
   *
   * Dump debug information about AutoTest's actions
   * in a chronological order into log files.
   * It automatically adds information about unit test
   * (unit test name) at every logging action.
   */
  class Logger:
    public Logging::SimpleLoggerHolder
  {
    typedef Logging::SimpleLoggerHolder Base;
    typedef std::vector<std::ofstream *> LogStreamsCont;

    /**
     * @brief List of file streams.
     *
     * Logger dumps information into that streams.
     */
    LogStreamsCont log_streams_;

    /// Auto test unit name.
    std::string log_name_;

    /// Auto test unit name.
    bool empty_;

  public:

    /**
     * @brief Constructor.
     *
     * Creates logger object and specify unit name.
     * Unit name used as aspect value for logger and
     * every log message forestalled with it.
     * @param log_name AutoTest log name.
     */
    Logger(const char* log_name);

    /**
     * @brief Gets logger trace level.
     * @return Returns current trace level.
     */
    unsigned long log_level() noexcept;
    
    /**
     * Clears list of loggers
     */
    virtual
    void
    clear_loggers() /*throw(eh::Exception)*/;

    /**
     * Check logger empty
     */
    bool
    empty() const noexcept;

    const std::string& log_name() const noexcept;

    /**
     * @brief Sets logger trace level.
     *
     * Records with severity value higher than trace
     * level will not be logged.
     * @param value Defines new log level.
     */
    void log_level(unsigned long value) noexcept;

    /**
     * @brief Logging.
     *
     * Logs text with severity and aspect specified.
     * @param text specifies text to be logged.
     * @param severity specify log record severity.
     * @return true on success.
     */
    template <typename Text>
    bool
    log(Text text,
      unsigned long severity = Logging::Logger::INFO)
      noexcept;

    /**
     * @brief Report error message.
     *
     * Logs text with severity ERROR.
     * ERROR severity represents 3rd log level.
     * If logger log level is lower than three
     * then text will not be logged.
     * @param text specifies text to be logged.
     * @return true on success.
     */
    bool
    error(const String::SubString& text)
      noexcept;

    /**
     * @brief Report warning message.
     *
     * Logs text with severity WARNING.
     * WARNING severity represents 4th log level.
     * If logger log level is lower than four
     * then text will not be logged.
     * @param text specifies text to be logged.
     * @return true on success.
     */
    bool
    warning(const String::SubString& text)
      noexcept;

    /**
     * @brief Report info message.
     *
     * Logs text with severity INFO.
     * INFO severity represents 6th log level.
     * If logger log level is lower than six
     * then text will not be logged.
     * @param text specifies text to be logged.
     * @return true on success.
     */
    bool
    info(const String::SubString& text)
      noexcept;

    /**
     * @brief Report debug message.
     *
     * Logs text with severity DEBUG.
     * DEBUG severity represents 7th log level.
     * If logger log level is lower than seven
     * then text will not be logged.
     * @param text specifies text to be logged.
     * @return true on success.
     */
    bool
    debug(const String::SubString& text)
      noexcept;

    /**
     * @brief Report trace message.
     *
     * Logs text with severity TRACE.
     * TRACE severity represents 8th log level.
     * If logger log level is lower than seven
     * then text will not be logged.
     * @param text specifies text to be logged.
     * @return true on success.
     */
    bool
    trace(const String::SubString& text)
      noexcept;

    /**
     * @brief Report message with very high log level.
     *
     * Used to log unimportant messages.
     * @param text specifies text to be logged.
     * @return true on success.
     */
    bool
    debug_trace(const String::SubString& text)
      noexcept;

    /**
     * thlog
     * @brief Get current thread logger.
     *
     * Get logger located in thread memory.
     * You may use result logger to log specific for this thread messages.
     * @return current thread logger.
     */
    static Logger& thlog();

    /**
     * thlog
     * @brief Set logger for current thread.
     *
     * Put logger into thread memory.
     * Then you can get it using thlog() function
     * and call its methods.
     * @param log logger to set.
     */
    static void thlog(Logger& log);

  protected:

    /**
     * @brief Destructor.
     */
    virtual ~Logger() noexcept;

  private:

    static Sync::Key<Logger> key_;
    
    /**
     * @brief Initializing.
     *
     * Initialize logger with output streams and other parameters.
     * @param params parameters for initialization.
     */
    void
    init_(Params params) /*throw(eh::Exception)*/;
  };

  typedef ReferenceCounting::SmartPtr<Logger> Logger_var;


  /**
   * @class LoggerSwitcher
   * @brief Switch test logs (if testcases used).
   */
  class LoggerSwitcher
  {
  public:
    /**
     * @brief Constructor.
     *
     * Switch to new & store old logger.
     * @param new logger
     */
    LoggerSwitcher(
      Logger& new_logger);

    /**
     * @brief Destructor.
     * Restore old logger
     */
    ~LoggerSwitcher();

  private:
    Logger& old_logger_;

  };
}

#include "Logger.ipp"

#endif //__AUTOTESTS_COMMONS_LOGGER_HPP

