
#ifndef _REPORT_HPP
#define _REPORT_HPP

#include "Statistics.hpp"
#include <eh/Exception.hpp>
#include <tests/PerformanceTests/Commons/ReportCommons.hpp>

/**
 * @class Report
 * @brief Base report
 */
class Report
{
public:

  /**
   * @brief Constructor.
   * @param statistics ref
   * @param output stream
   */
  Report(Statistics& stats,
         std::ostream& out);
  
  /**
   * @brief Destructor
   */
  virtual ~Report() noexcept;

  /**
   * @brief Publish report
   */
  virtual void publish() = 0;

  /**
   * @brief Dump report to output stream
   */
  virtual void dump();

protected:
  Statistics& stats_;
  std::ostream& out_;

};

/**
 * @class DumpRequestContainerConfStats
 * @brief Class for dumping counters container with per/second info
 */
class DumpRequestContainerConfStats : public DumpInterface
{
public:
 
  /**
   * @brief Constructor.
   * @param statistics ref
   */
   DumpRequestContainerConfStats(const RequestCounterContainer& container);

  /**
   * @brief Destructor
   */
  ~DumpRequestContainerConfStats() noexcept;

protected:
  /**
   * @brief dump body
   * @param output stream
   */  
  virtual std::ostream& dump_body(std::ostream& out) const;  
  
private:
  const RequestCounterContainer& container_;
};


/**
 * @class DumpFrontendStats
 * @brief Class for dumping frontend statistics
 */
class DumpFrontendStats : public DumpInterface
{
public:
  static const unsigned short INDENTION = 30;

  static const unsigned short LINE_LENGTH = 120;
  
public:
   /**
   * @brief Constructor.
   * @param statistics ref
   */
  DumpFrontendStats(const FrontendStatList& items);

  /**
   * @brief Destructor
   */
  ~DumpFrontendStats() noexcept;

protected:

  /**
   * @brief dump header
   * @param output stream
   */  
  virtual std::ostream& dump_header(std::ostream& out) const;

  /**
   * @brief dump body
   * @param output stream
   */  
  virtual std::ostream& dump_body(std::ostream& out) const;

  
private:
  const FrontendStatList& items_;   
};

/**
 * @class DumpAdvertisingStats
 * @brief Class for dumping advertising statistics
 */
class  DumpAdvertisingStats : public DumpInterface
{

public:
  static const unsigned short INDENTION = 30;
  
  static const unsigned short LINE_LENGTH = 120;

public:
  
   /**
   * @brief Constructor.
   * @param statistics ref
   */
   DumpAdvertisingStats(const AdvertisingStatList& items);

  /**
   * @brief Destructor
   */
  ~DumpAdvertisingStats() noexcept;

protected:

  /**
   * @brief dump body
   * @param output stream
   */  
  virtual std::ostream& dump_body(std::ostream& out) const;
  
private:
  const AdvertisingStatList& items_;

private:
  std::ostream& dump_ccid_header(std::ostream& out) const;

  std::ostream& dump_ccid_body(std::ostream& out,
                               const AdvertisingStatistics_var& stats) const;

};

/**
 * @class ShortReport
 * @brief Short statistics form report
 */
class ShortReport : public Report
{
public:
  /**
   * @brief Constructor.
   * @param statistics ref
   * @param output stream
   */
  ShortReport(Statistics& stats,
              std::ostream& out);

  /**
   * @brief Destructor
   */
  ~ShortReport() noexcept;

  /**
   * @brief Publish report
   */
  virtual void publish();

  /**
   * @brief dump
   */  
  virtual void dump();
   
};

/**
 * @class ChannelsDump
 * @brief Channels statistics report
 */
class ChannelsReport : public  Report
{
public:
  /**
   * @brief Constructor.
   * @param statistics ref
   * @param output stream
   */
  ChannelsReport(Statistics& stats,
                 std::ostream& out);

  /**
   * @brief Destructor
   */
  ~ChannelsReport() noexcept;

  /**
   * @brief Publish report
   */
  virtual void publish();


  /**
   * @brief dump
   */  
  virtual void dump();  
 
};


/**
 * @class ChannelsReport
 * @brief Channels statistics report
 */
class DiscoverReport : public Report
{
public:
  /**
   * @brief Constructor.
   * @param statistics ref
   * @param output stream
   */
  DiscoverReport(Statistics& stats,
                 std::ostream& out);

  /**
   * @brief Destructor
   */
  ~DiscoverReport() noexcept;

  /**
   * @brief dump
   */  
  virtual void publish();

  /**
   * @brief dump
   */  
  virtual void dump();  
 
};


/**
 * @class StandardReport
 * @brief Using for statistics logging
 */
class StandardReport : public Report
{
public:

  /**
   * @brief Constructor.
   * @param statistics ref
   * @param output stream
   */
  StandardReport(Statistics& stats,
                 std::ostream& out);

  /**
   * @brief Destructor
   */
  ~StandardReport() noexcept;

  /**
   * @brief dump
   */  
  virtual void publish();  

private:
  
  virtual std::ostream& dump_frontend_footer(std::ostream& out);
 
};

/**
 * @class DumpPerformanceStats
 * @brief Class for dumping performance statistics
 */
class DumpConfPerformanceStats : public DumpInterface
{
 
public:

  /**
   * @brief Constructor.
   * @param statistics ref
   * @param performance statistics header
   */
  DumpConfPerformanceStats(const PerformanceStatisticsBase& stats,
                           const char* header);

  /**
   * @brief Destructor
   */
  ~DumpConfPerformanceStats() noexcept;

protected:

  /**
   * @brief dump header
   * @param output stream
   */  
  virtual std::ostream& dump_header(std::ostream& out) const;

  /**
   * @brief dump body
   * @param output stream
   */  
  virtual std::ostream& dump_body(std::ostream& out) const;  
  
private:
  const PerformanceStatisticsBase& stats_;
  std::string header_;
};


class ConfluenceReport : public Report
{
public:

  DECLARE_EXCEPTION(ConfReportError, eh::DescriptiveException);      
  /**
   * @brief Constructor.
   * @param statistics ref
   * @param report name
   * @param report file path
   */
  ConfluenceReport(Statistics& stats,
                   const Configuration& config,
                   const ConstraintsContainer& constraints,
                   Generics::Time total_duration);

  /**
   * @brief Destructor
   */
  ~ConfluenceReport() noexcept;

  /**
   * @brief dump
   */  
  virtual void publish();

private:
  const Configuration& config_;
  const ConstraintsContainer& constraints_;
  Generics::Time total_duration_;
  std::ofstream file_;
  
private:
  bool is_file_exist();
  void dump_header(std::ostream& out);
  void dump_body(std::ostream& out);
  void dump_server_list(std::ostream& out);
};



#endif  // _REPORT_HPP
