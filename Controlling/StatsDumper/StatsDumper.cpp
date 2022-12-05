/// @file Controlling/StatsDumper/StatsDumper.cpp
#include <Generics/Network.hpp>
#include "StatsDumper.hpp"

namespace AdServer
{
  namespace Controlling
  {
    StatsDumper::StatsDumper(
      const CORBACommons::CorbaObjectRef& ref,
      const char* host_name)
      /*throw(Exception)*/
      : ref_(CORBACommons::CorbaClientAdapter_var(
          new CORBACommons::CorbaClientAdapter()).in(), ref)
    {
      static const char* FUN = "StatsDumper::StatsDumper";

      try
      {
        if (!host_name)
        {
          char name[2048];
          if (::gethostname(name, sizeof(name)) != 0)
          {
            eh::throw_errno_exception<Exception>(errno,
              FUN, "gethostname failed on host name '", name, "'");
          }
          host_name_ = name;
        }
        else
        {
          host_name_ = host_name;
        }
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN
          << ": Controlling::StatsCollector::can't init: " << e.what();
        throw Exception(ostr);
      }
      catch (const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught exception on _narrow: " << e;
        throw Exception(ostr);
      }
    }

    void
    StatsDumper::dump_statistics(const Generics::Values& stat_snapshot)
      /*throw(Exception)*/
    {
      static const char* FUN = "StatsDumper::dump_statistic";

      try
      {
        StatsValueSeq_var in(
          CORBACommons::ValuesConverter::get_stats(stat_snapshot));
        ref_->add_stats(host_name_.c_str(), in);
      }
      catch (const CORBACommons::ProcessStatsControl::ImplementationException&
        e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught "
          "CORBACommons::ProcessStatsControl::ImplementationException: "
          << e.description;
        throw Exception(ostr, "ADS-ICON-4");
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr  << FUN << ": caught CORBA::SystemException: " << e;
        throw Exception(ostr, "ADS-ICON-4");
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr  << FUN << ": caught eh::Exception: " << e.what();
        throw Exception(ostr);
      }
    }

  }
}
