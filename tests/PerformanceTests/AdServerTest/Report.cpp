
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Report.hpp"



// Class Report

Report::Report(Statistics& stats,
               std::ostream& out) :
  stats_(stats),
  out_(out)
{ }

Report::~Report() noexcept
{ }

void Report::dump()
{
  stats_.refresh(this);
}



// Class DumpRequestContainerConfStats
DumpRequestContainerConfStats::DumpRequestContainerConfStats
(const RequestCounterContainer& container) :
    container_(container)
{ }

DumpRequestContainerConfStats::~DumpRequestContainerConfStats() noexcept
{ }


std::ostream& DumpRequestContainerConfStats::dump_body(std::ostream& out) const
{
  out << DumpPercentageCounter(container_.total()) <<
      " \\\\  -- *OI*=" <<
      DumpPercentageCounter(container_.opted_in()) <<
      " \\\\  -- *OO*=" <<
      DumpPercentageCounter(container_.opted_out());
  return out;
}

// Class DumpFrontendStats

DumpFrontendStats::DumpFrontendStats(const FrontendStatList& items)
  : items_(items)
{ }

DumpFrontendStats::~DumpFrontendStats() noexcept
{ }

std::ostream& DumpFrontendStats::dump_header(std::ostream& out) const
{
  print_line(out, LINE_LENGTH);
  out.fill (' ');
  out.flags(std::ios::right);
  print_header(out, "Frontend statistics", LINE_LENGTH);
  out.width(LINE_LENGTH - 3*INDENTION);
  out << "Frontend";
  out.width (INDENTION);
  out << "Requests(%)";
  out.width (INDENTION);
  out << "Failed(%)";
  out.width (INDENTION);
  out << "Approved(%)";
  out << std::endl;
  print_line(out, LINE_LENGTH);
  return out;
}

std::ostream& DumpFrontendStats::dump_body(std::ostream& out) const
{
  for (FrontendStatList::const_iterator it = items_.begin();
       it != items_.end(); ++it)
  {
    const FrontendStatistics_var& item = it->second;
    out.fill (' ');
    out.flags(std::ios::right);
    // Total
    out.width(LINE_LENGTH - 3*INDENTION - 2);
    out <<  item->frontend_name();
    out.width (INDENTION);
    out << DumpPercentageCounter(item->requests().total());
    out.width (INDENTION);
    out << DumpPercentageCounter(item->errors().total());
    out.width (INDENTION);
    out << DumpPercentageCounter(item->approved().total());
    out << std::endl;

    // Opted in
    out.width(LINE_LENGTH - 3*INDENTION );
    out <<  "OI";
    out.width (INDENTION);
    out << DumpPercentageCounter(item->requests().opted_in());
    out.width (INDENTION);
    out << DumpPercentageCounter(item->errors().opted_in());
    out.width (INDENTION);
    out << DumpPercentageCounter(item->approved().opted_in());
    out << std::endl;

    // Opted out
    out.width(LINE_LENGTH - 3*INDENTION);
    out <<  "OO";
    out.width (INDENTION);
        out << DumpPercentageCounter(item->requests().opted_out());
    out.width (INDENTION);
    out << DumpPercentageCounter(item->errors().opted_out());
    out.width (INDENTION);
    out << DumpPercentageCounter(item->approved().opted_out());
    out << std::endl;
  }


  return out;
}

// Class DumpAdvertisingStats

DumpAdvertisingStats::DumpAdvertisingStats(const AdvertisingStatList& items) :
  items_(items)
{ }

DumpAdvertisingStats::~DumpAdvertisingStats() noexcept
{ }

std::ostream& DumpAdvertisingStats::dump_body(std::ostream& out) const
{
  for (AdvertisingStatList::const_iterator it = items_.begin();
       it != items_.end(); ++it)
  {
    print_line(out, LINE_LENGTH);
    std::string header("Advertising statistics ");
    header+= "('" + it->first + "')";
    print_header(out, header.c_str(), LINE_LENGTH);
    dump_ccid_header(out);
    out << std::endl;
    print_line(out, LINE_LENGTH);
    dump_ccid_body(out, it->second);
    out << std::endl;
    print_line(out, LINE_LENGTH);
    out << std::endl << std::endl;
  }
  return out;
}

std::ostream& DumpAdvertisingStats::dump_ccid_header(std::ostream& out) const
{
  out.flags(std::ios::right);
  out.width (INDENTION);
  out.fill (' ');
  out << std::right << "CCID";
  out.width (INDENTION);
  out << std::right << "Total(%)";
  out.width (INDENTION);
  out << std::right << "Opted in(%)";
  out.width (INDENTION);
  out << std::right << "Opted out(%)";
  return out;
}

std::ostream& DumpAdvertisingStats::dump_ccid_body(std::ostream& out,
                                                   const AdvertisingStatistics_var& stats) const
{
 out.flags(std::ios::right);
 for(AdvertisingStatistics::RequestCounterContainerList::const_iterator
       it=stats->ccid_stats().begin();
     it != stats->ccid_stats().end(); ++it)
    {
      out.width (INDENTION);
      out.fill (' ');
      out << it->first;
      out.width (INDENTION);
      out << DumpPercentageCounter(it->second->total());
      out.width (INDENTION);
      out << DumpPercentageCounter(it->second->opted_in());
      out.width (INDENTION);
      out << DumpPercentageCounter(it->second->opted_out());
      out << std::endl;
    }
  print_line(out, LINE_LENGTH);
  out.width (INDENTION);
  out << "Return creatives";
  out.width (INDENTION);
  out << stats->creative_responses().total().get();
  out.width (INDENTION);
  out << DumpPercentageCounter(stats->creative_responses().opted_in());
  out.width (INDENTION);
  out << DumpPercentageCounter(stats->creative_responses().opted_out());
  out << std::endl;
  out.width (INDENTION);
  out << "Total";
  out.width (INDENTION);
  out << stats->total_responses().total().get();
  out.width (INDENTION);
  out << DumpPercentageCounter(stats->total_responses().opted_in());
  out.width (INDENTION);
  out << DumpPercentageCounter(stats->total_responses().opted_out());
  return out;
}



// Class ShortDump

ShortReport::ShortReport(Statistics& stats,
                         std::ostream& out) :
  Report(stats, out)
{ }

ShortReport::~ShortReport() noexcept
{ }

void ShortReport::publish()
{
  out_ << "Common stats: requests: " <<
      DumpCounter(stats_.total_requests().total()) <<
      ", approved: " << DumpCounter(stats_.total_approved().total()) <<
      ", failed: " << DumpCounter(stats_.total_errors().total());
}

void ShortReport::dump()
{
  stats_.refresh(this, Statistics::RST_COUNTERS);
}

// Class ChannelsReport

ChannelsReport::ChannelsReport(Statistics& stats,
                               std::ostream& out) :
  Report(stats, out)
{ }

ChannelsReport::~ChannelsReport() noexcept
{ }

void ChannelsReport::publish()
{
  out_ << "Channel usage: " <<
      DumpRangeStats(stats_.request_channels_stats());
}

void ChannelsReport::dump()
{
  stats_.refresh(this, Statistics::RST_CHANNELS);
}

// Class StandardReport

StandardReport::StandardReport(Statistics& stats,
                               std::ostream& out) :
  Report(stats, out)
{ }

StandardReport::~StandardReport() noexcept
{ }


void StandardReport::publish()
{
  // Frontend statistics output
  out_ << DumpFrontendStats(stats_.frontend_stats());
  dump_frontend_footer(out_);

  // Adverstising performance statistics output
  out_ <<  DumpPerformanceStats(stats_.adv_performance_stats(),
                                 "Advertising performance statistics") <<
    // Advertising statistics output
    DumpAdvertisingStats(stats_.adv_stats());
}

std::ostream& StandardReport::dump_frontend_footer(std::ostream& out)
{

  // Profiling
  out.width (DumpFrontendStats::LINE_LENGTH - 3*DumpFrontendStats::INDENTION - 2);
  out << "Profiling";
  out.width (DumpFrontendStats::INDENTION);
  out << DumpPercentageCounter(stats_.profiling_requests().total());
  out << std::endl;
  out.width (DumpFrontendStats::LINE_LENGTH - 3*DumpFrontendStats::INDENTION);
  out << "OI";
  out.width (DumpFrontendStats::INDENTION);
  out << DumpPercentageCounter(stats_.profiling_requests().opted_in());
  out << std::endl;
  out.width (DumpFrontendStats::LINE_LENGTH - 3*DumpFrontendStats::INDENTION);
  out << "OO";
  out.width (DumpFrontendStats::INDENTION);
  out << DumpPercentageCounter(stats_.profiling_requests().opted_out());
  out << std::endl;

  print_line(out, DumpFrontendStats::LINE_LENGTH);
  out.fill (' ');
  out.flags(std::ios::right);

  // Total
  out.width (DumpInterface::LINE_LENGTH - 3*DumpFrontendStats::INDENTION - 2);
  out << "Total";
  out.width (DumpFrontendStats::INDENTION);
  out << stats_.total_requests().total().get();
  out.width (DumpFrontendStats::INDENTION);
  out << stats_.total_errors().total().get();
  out.width (DumpFrontendStats::INDENTION);
  out << stats_.total_approved().total().get();
  out << std::endl;

  // Opted in
  out.width (DumpFrontendStats::LINE_LENGTH - 3*DumpFrontendStats::INDENTION);
  out << "OI";
  out.width (DumpFrontendStats::INDENTION);
  out << DumpPercentageCounter(stats_.total_requests().opted_in());
  out.width (DumpFrontendStats::INDENTION);
  out << DumpPercentageCounter(stats_.total_errors().opted_in());
  out.width (DumpFrontendStats::INDENTION);
  out << DumpPercentageCounter(stats_.total_approved().opted_in());
  out << std::endl;

  // Opted out
  out.width (DumpFrontendStats::LINE_LENGTH - 3*DumpFrontendStats::INDENTION);
  out << "OO";
  out.width (DumpFrontendStats::INDENTION);
  out << DumpPercentageCounter(stats_.total_requests().opted_out());
  out.width (DumpFrontendStats::INDENTION);
  out << DumpPercentageCounter(stats_.total_errors().opted_out());
  out.width (DumpFrontendStats::INDENTION);
  out << DumpPercentageCounter(stats_.total_approved().opted_out());
  out << std::endl;

  print_line(out, DumpFrontendStats::LINE_LENGTH);
  out << std::endl << std::endl;
  return out;
}

// Class DumpConfPerformanceStats

DumpConfPerformanceStats::DumpConfPerformanceStats(const PerformanceStatisticsBase& stats,
                                                   const char* header) :
  stats_(stats),
  header_(header)
{ }

DumpConfPerformanceStats::~DumpConfPerformanceStats() noexcept
{ }

std::ostream& DumpConfPerformanceStats::dump_header(std::ostream& out) const
{
  out << "{color:#0000ff}{+}" << header_ << "{+}{color} \\\\";
  return out;
}


std::ostream& DumpConfPerformanceStats::dump_body(std::ostream& out) const
{
  for (PerformanceStatisticsBase::ItemList::const_iterator
           it = stats_.items().begin();
       it != stats_.items().end(); ++it)
  {

    out << " *" << it->first << "* \\\\ (";
    print_time(out, it->second->min());
    out << ", ";
    print_time(out, it->second->max());
    out << ", ";
    print_time(out, it->second->average());
    out << ") \\\\";
  }
  return out;
}

// Class ConfluenceReport

ConfluenceReport::ConfluenceReport(Statistics& stats,
                                   const Configuration& config,
                                   const ConstraintsContainer& constraints,
                                   Generics::Time total_duration) :
  Report(stats, file_),
  config_(config),
  constraints_(constraints),
  total_duration_(total_duration)
{ }

ConfluenceReport::~ConfluenceReport() noexcept
{ }

void ConfluenceReport::publish()
{
  bool file_exists = is_file_exist();
  file_.open(config_.confluence_report_path().c_str(), std::ios::app);
  if (file_.fail())
  {
    Stream::Error err;
    err << "Can't open file '" << config_.confluence_report_path() << "'";
    throw ConfReportError(err);
  }
  try
  {
    if (!file_exists)
        dump_header(file_);
    dump_body(file_);
  }
  catch (...)
  {
    file_.close();
    throw;
  }
  file_.close();
}

bool ConfluenceReport::is_file_exist()
{
  struct stat st;
  return stat(config_.confluence_report_path().c_str(),&st) == 0;
}

void ConfluenceReport::dump_header(std::ostream& out)
{
  out << "|| Date || Host || Configuration params || Request Channels (min/max/avg) "
      "|| Result || Common Statistics || Performance statistics \\\\        \\\\"
      "name (min; max; avg) || Frontend stats \\\\ (min/max/avg) ||" << std::endl;
}

void ConfluenceReport::dump_body(std::ostream& out)
{
  // Date
  out << "| " << Generics::Time::get_time_of_day().get_local_time().format("%d.%m.%Y") << " | ";
  // Host
  dump_server_list(out);
  out << " | "<< config_.description() << " | ";
  // Request channels
  out << DumpRangeStats(stats_.request_channels_stats(), true) << " | ";
  // Result (constraints)
  out << constraints_.error() << " | ";
  // Common stats
  out << "*Total requests*=" <<
      DumpRequestContainerConfStats(stats_.total_requests()) <<
      " \\\\  *Errors*=" <<
      DumpRequestContainerConfStats(stats_.total_errors());
  // Frontend stats
  for (FrontendStatList::const_iterator it = stats_.frontend_stats().begin();
       it != stats_.frontend_stats().end(); ++it)
  {
    const FrontendStatistics_var& item = it->second;
    out << " \\\\  *"<< it->first << "*=" <<
        DumpRequestContainerConfStats(item->requests());
  }
  // Creative stats
  AdvertisingStatistics* ns_stats = stats_.get_nslookup_stats();
  if (ns_stats)
  {
    out << " \\\\  *Return creatives*=" <<
        DumpRequestContainerConfStats(ns_stats->creative_responses());
  }
  out << " \\\\  *Profiling*=" <<
      DumpRequestContainerConfStats(stats_.profiling_requests());
  out << "\\\\  *Execution(secs)*=" << total_duration_ << " | ";
  // Performance stats
  out << DumpConfPerformanceStats(stats_.adv_performance_stats(), "Advertising") << " | ";
  // Frontend stats
  out << " |";
}

void ConfluenceReport::dump_server_list(std::ostream& out)
{
  unsigned int size = config_.server_urls()->size();
  for (unsigned int i = 0; i < size; ++i)
  {
    std::string url;
    config_.server_urls()->get(i, url);
    out << url;
    if (i < size - 1)
        out << ", ";
  }
}




