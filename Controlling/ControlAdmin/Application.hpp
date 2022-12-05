#ifndef CONTROL_ADMIN_APPLICATION_HPP_
#define CONTROL_ADMIN_APPLICATION_HPP_

#include <getopt.h>
#include<limits.h>
#include<vector>
#include<string>
#include<iostream>
#include<sstream>
#include<eh/Exception.hpp>
#include<CORBACommons/CorbaAdapters.hpp>
#include<CORBACommons/Stats.hpp>
#include<Controlling/StatsCollector/StatsCollector.hpp>
#include<UtilCommons/Table.hpp>

namespace AdServer
{
  namespace Controlling
  {
    class ControlAdmin
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      ControlAdmin() noexcept
        : adapter_(new CORBACommons::CorbaClientAdapter), quiet_(false)
      {
      }
      virtual ~ControlAdmin()noexcept{};
      int run(int argc, char* argv[]) noexcept;
      void help(int topic) noexcept;
      int get(int argc, char* argv[]) noexcept;
      int put(int argc, char* argv[]) noexcept;

    private:
      enum
      {
        GET_TOPIC = 0,
        PUT_TOPIC,
        MAX_TOPIC
      };
      enum
      {
        HELP_COM = 0,
        GET_COM,
        PUT_COM,
        MAX_COM
      };

      enum Types
      {
        LONG = 0,
        ULONG,
        DOUBLE,
        STRING
      };

      void parse_options_(int argc, char* argv[]) noexcept;

      void resolve_stats_control_ref_() /*throw(Exception)*/;

      void print_header_() noexcept;
      void print_keys_() /*throw(Exception)*/;

      static Types detect_value_(const std::string& str) noexcept;

    private:
      typedef std::vector<std::string> StringVector;
      typedef std::unique_ptr<Table> TablePtr;

      StringVector keys_;
      StringVector values_;
      std::string reference_;
      CORBACommons::CorbaClientAdapter_var adapter_;
      StatsCollector_var service_;
      bool quiet_;
      std::string command_;

    private:
      static const char* topics[MAX_TOPIC+1];
      static const char* command[MAX_COM];
    };
  }
}

#endif
