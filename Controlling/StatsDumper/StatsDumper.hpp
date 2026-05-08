/// @file Controlling/StatsDumper/StatsDumper.hpp
#pragma once

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
