/// @file Controlling/StatsDumper/StatsDumper.hpp
#ifndef CONTROLLING_STATS_DUMPER_HPP_
#define CONTROLLING_STATS_DUMPER_HPP_

#include <eh/Exception.hpp>

#include <CORBACommons/StatsImpl.hpp>
#include <Commons/CorbaObject.hpp>
#include <Controlling/StatsCollector/StatsCollector.hpp>

namespace AdServer
{
  namespace Controlling
  {
    class StatsDumper
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      StatsDumper(
        const CORBACommons::CorbaObjectRef& ref,
        const char* host_name = 0)
        /*throw(Exception)*/;

      void
      dump_statistics(const Generics::Values& stat_snapshot)
        /*throw(Exception)*/;

    private:
      std::string host_name_;
      Commons::CorbaObject<StatsCollector> ref_;
    };

  }
}

#endif //CONTROLLING_STATS_DUMPER_HPP_
