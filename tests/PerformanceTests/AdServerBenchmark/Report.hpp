
#ifndef _REPORT_HPP
#define _REPORT_HPP

#include "Statistics.hpp"
#include <tests/PerformanceTests/Commons/ReportCommons.hpp>
#include <xsd/tests/PerformanceTests/AdServerBenchmark/AdServerBenchmarkConfig.hpp>


/**
 * @class Report
 * @brief Benchmark report
 */
class Report
{

  static const unsigned long INDENTION=30;

public:

  /**
   * @brief Constructor.
   * @param benchmark description
   * @param frontend type
   * @param statistics reference
   * @param output stream
   */
  Report(const char* description,
         FrontendType frontend_type,
         Statistics& stats,
         std::ostream& out);

  /**
   * @brief Dump report to output stream
   */
  void dump();

private:
  std::string description_;     // description
  FrontendType frontend_type_;  // frontend type
  Statistics& stats_;           // statistics
  std::ostream& out_;           // output stream
};


#endif  // _REPORT_HPP
