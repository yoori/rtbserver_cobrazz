
// Class Report

#include "Report.hpp"

Report::Report(const char* description,
               FrontendType frontend_type,
               Statistics& stats,
               std::ostream& out) :
  description_(description),
  frontend_type_(frontend_type),
  stats_(stats),
  out_(out)
{
}

void Report::dump()
{
  Generics::Time duration = stats_.duration();
  double seconds = duration.as_double();
  out_.precision(2);
  out_.fill (' ');
  out_.flags(std::ios::left);
  out_ << std::endl << description_ <<
      " results:  " << std::endl;
  out_.width (INDENTION);
  out_ << "  Total requests: ";
  out_ << stats_.responses() << std::endl;
  out_.width (INDENTION);
  out_  << "  Errors: ";
  out_ << stats_.errors() << std::endl;
  if (stats_.creatives())
  {
    out_.width (INDENTION);
    out_ << "  Advertising: ";
    out_ << stats_.creatives() << std::endl;
  }
  out_.width (INDENTION);
  out_ << "  Clients: ";
  out_ << stats_.uids_size() << std::endl;
  if (frontend_type_ == FrontendType::nslookup &&
    stats_.history_channels_stats().get())
  {
    out_.width (INDENTION);
    out_ << "  History channels: ";
    out_ << DumpRangeStats(stats_.history_channels_stats()) << std::endl;
  }
  if (frontend_type_ == FrontendType::nslookup &&
    stats_.trigger_channels_stats().get())
  {
    out_.width (INDENTION);
    out_ << "  Trigger channels: ";
    out_ << DumpRangeStats(stats_.trigger_channels_stats()) << std::endl;
  }
  for (PerformanceStatisticsBase::ItemList::const_iterator
         it = stats_.adv_performance_stats().items().begin();
       it != stats_.adv_performance_stats().items().end(); ++it)
    {
      std::string header("  " +  it->first + ": ");
      out_.width (INDENTION);
      out_ << header;
      print_time(out_, it->second->min());
      out_ << "/";
      print_time(out_, it->second->max());
      out_ << "/";
      print_time(out_, it->second->average());
      out_ << std::endl;
    }
  out_.width (INDENTION);
  out_ << "  Execution time: ";
  print_time(out_, duration);
  out_ << " (sec)" << std::endl;
  out_.width (INDENTION);
  out_ << "  Requests per second: ";
  out_  << std::fixed <<
      static_cast<double>(stats_.responses()) / seconds << std::endl;
}
