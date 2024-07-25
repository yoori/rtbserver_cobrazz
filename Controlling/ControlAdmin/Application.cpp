#include <getopt.h>
#include<unistd.h>
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
#include<Controlling/StatsDumper/StatsDumper.hpp>
#include"Application.hpp"

namespace AdServer
{
  namespace Controlling
  {
    Table::Column HEADER_FIELDS[] =
    {
      Table::Column("query"),
      Table::Column("query_date"),
      Table::Column("reference")
    };

    Table::Column DATA_FIELDS[] =
    {
      Table::Column("key"),
      Table::Column("value"),
    };

    const char* ControlAdmin::topics[MAX_TOPIC+1] =
    {
      "Get: ControlAdmin get -r reference [-q]\n",
      "Put: ControlAdmin put -r reference -k key -v value [-q]\n",
      "Usage: ControlAdmin [get|put] -r [reference] [-q] [-k key -v value...]\n"
    };

    const char* ControlAdmin::command[MAX_COM] =
    {
      "Help",
      "Get",
      "Put"
    };

    void ControlAdmin::print_header_() noexcept
    {
      if(quiet_)
      {
        return;
      }
      const size_t count_fields =
        sizeof(HEADER_FIELDS)/sizeof(HEADER_FIELDS[0]);
      std::unique_ptr<Table> table(new Table(count_fields));
      for(size_t i = 0; i < count_fields; i++)
      {
        table->column(i, HEADER_FIELDS[i]);
      }
      Table::Row row(count_fields);
      row.add_field(command_);
      row.add_field(Generics::Time::get_time_of_day().get_gm_time().format(
          "%Y-%m-%d-%H:%M:%S"));
      row.add_field(reference_);
      table->add_row(row);
      table->dump(std::cout);
    }

    void ControlAdmin::print_keys_() /*throw(Exception)*/
    {
      if(keys_.size() != values_.size())
      {
        Stream::Error err;
        err << "ControlAdmin::print_keys_: size of keys = "
          << keys_.size() << " not equal size of values = " << values_.size();
        throw Exception(err);
      }
      const size_t count_fields =
        sizeof(DATA_FIELDS)/sizeof(DATA_FIELDS[0]);
      std::unique_ptr<Table> table(new Table(count_fields));
      for(size_t i = 0; i < count_fields; i++)
      {
        table->column(i, DATA_FIELDS[i]);
      }
      Table::Row row(count_fields);
      for(size_t i = 0; i < keys_.size(); i++)
      {
        row.add_field(keys_[i]);
        row.add_field(values_[i]);
        table->add_row(row);
        row.clear();
      }
      table->dump(std::cout);
    }

    void ControlAdmin::resolve_stats_control_ref_() /*throw(Exception)*/
    {
      try
      {
        if(reference_.empty())
        {
          throw Exception("Empty corba reference");
        }
        CORBA::Object_var obj_ref = adapter_->resolve_object(reference_.c_str());
        if (CORBA::is_nil(obj_ref))
        {
          throw Exception("Corba reference is null");
        }
        service_ = StatsCollector::_narrow(obj_ref.in());
        if (CORBA::is_nil(service_))
        {
          throw Exception("Can't resolve corba reference, narrow return null");
        }
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error err;
        err << e;
        throw Exception(err);
      }
      catch(const eh::Exception& e)
      {
        throw Exception(e.what());
      }
    }

    void ControlAdmin::parse_options_(int argc, char* argv[]) noexcept
    {
      struct option long_options[] =
      {
        {"reference", required_argument, 0, 'r'},
        {"key", required_argument, 0, 'k'},
        {"value", required_argument, 0, 'v'},
        {"quiet", required_argument, 0, 'q'},
        {0, 0, 0, 0}
      };
      int opt, index=0;
      do
      {
        opt = getopt_long(argc, argv, "qr:k:v:", long_options, &index);
        switch(opt)
        {
          case -1:
          break;
          case 'r':
            reference_ = optarg;
            break;
          case 'k':
            keys_.push_back(optarg);
            break;
          case 'v':
            values_.push_back(optarg);
            break;
          case 'q':
            quiet_ = true;
            break;
          case '?':
            break;
        }
      } while(opt!=-1);
    }

    ControlAdmin::Types ControlAdmin::detect_value_(
      const std::string& str)
      noexcept
    {
      if(str.size())
      {
        bool isNeg = false, wasPoint = false;
        size_t i=0;
        if(str[0] == '-')
        {
          isNeg = true;
          i++;
        }
        char point =
          std::use_facet<std::numpunct<char> >(std::locale()).decimal_point();
        for(;i<str.size();i++)
        {
          if(str[i] == point)
          {
            if(wasPoint)
            {
              return STRING;
            }
            wasPoint = true;
            continue;
          }
          if(!isdigit(str[i]))
          {
            return STRING;
          }
        }
        if(wasPoint)
        {
          return DOUBLE;
        }
        if(isNeg)
        {
          return LONG;
        }
        Stream::Parser istr(str);
        unsigned long res;
        istr >> res;
        if(res>LONG_MAX)
        {
          return ULONG;
        }
        return LONG;
      }
      return STRING;
    }

    void ControlAdmin::help(int topic) noexcept
    {
      if(topic>MAX_TOPIC)
      {
        topic = MAX_TOPIC;
      }
      std::cout << topics[topic];
    }

    int ControlAdmin::get(int argc, char* argv[]) noexcept
    {
      try
      {
        parse_options_(argc, argv);
        resolve_stats_control_ref_();
        print_header_();
        CORBACommons::StatsValueSeq_var values = service_->get_stats();
        keys_.clear();
        values_.clear();
        keys_.reserve(values->length());
        values_.reserve(values->length());
        for(size_t i = 0; i < values->length(); i++)
        {
          const CORBACommons::StatsValue& cur_value = (*values)[i];
          Stream::Error str_value;
          keys_.push_back(cur_value.key.in());
          if (cur_value.value.type()->equal(CORBA::_tc_longlong))
          {
            CORBA::LongLong real_value;
            cur_value.value >>= real_value;
            str_value << real_value;
          } else if (cur_value.value.type()->equal(CORBA::_tc_ulonglong))
          {
            CORBA::ULongLong real_value;
            cur_value.value >>= real_value;
            str_value << real_value;
          } else if(cur_value.value.type()->equal(CORBA::_tc_long))
          {
            CORBA::Long real_value;
            cur_value.value >>= real_value;
            str_value << real_value;
          } else if(cur_value.value.type()->equal(CORBA::_tc_ulong))
          {
            CORBA::ULong real_value;
            cur_value.value >>= real_value;
            str_value << real_value;
          } else if(cur_value.value.type()->equal(CORBA::_tc_double))
          {
            CORBA::Double real_value;
            cur_value.value >>= real_value;
            // TODO maybe not 6
            // std::fixed(str_value);
            str_value << Stream::MemoryStream::double_out(real_value, 6);
          } else if(cur_value.value.type()->equal(CORBA::_tc_string))
          {
            const char* real_value = 0;
            cur_value.value >>= real_value;
            str_value << real_value;
          }
          values_.push_back(str_value.str().str());
        }
        print_keys_();
      }
      catch(const CORBACommons::ProcessStatsControl::ImplementationException& e)
      {
        std::cerr << e.description << std::endl;
        return 1;
      }
      catch(const CORBA::SystemException& e)
      {
        std::cerr << e << std::endl;
        return 1;
      }
      catch(const Exception& e)
      {
        std::cerr << e.what() << std::endl;
        return 1;
      }
      return 0;
    }

    int ControlAdmin::put(int argc, char* argv[]) noexcept
    {
      try
      {
        parse_options_(argc, argv);
        resolve_stats_control_ref_();
        if(keys_.empty() || values_.empty())
        {
          std::cerr << "keys or values didn't set\n" << std::endl;
          return 1;
        }
        print_header_();
        print_keys_();

        size_t count = keys_.size();

        StatsValueSeq in;
        in.length(count);
        for(size_t i=0;i<count;i++)
        {
          in[i].key << keys_[i];
          switch(detect_value_(values_[i]))
          {
            case LONG:
              in[i].value <<= static_cast<CORBA::LongLong>(
                strtol(values_[i].c_str(), 0, 10));
              break;
            case ULONG:
              in[i].value <<= static_cast<CORBA::ULongLong>(
                strtoul(values_[i].c_str(), 0, 10));
              break;
            case DOUBLE:
              in[i].value <<= strtod(values_[i].c_str(), 0);
              break;
            default:
              in[i].value <<= values_[i].c_str();
              break;
          }
        }
        char name[2048];
        if(gethostname(name, sizeof(name))!=0)
        {
          std::cerr << strerror(errno);
        }
        service_->add_stats(name, in);
      }
      catch(const CORBACommons::ProcessStatsControl::ImplementationException& e)
      {
        std::cerr << e.description << std::endl;
        return 1;
      }
      catch(const CORBA::SystemException& e)
      {
        std::cerr << e << std::endl;
        return 1;
      }
      catch(const Exception& e)
      {
        std::cerr << e.what() << std::endl;
        return 1;
      }
      return 0;
    }

    int ControlAdmin::run(int argc, char* argv[]) noexcept
    {
      if(argc > 1)
      {
        command_ = argv[1];
        if(!strcasecmp(command_.c_str(), command[HELP_COM]))
        {
          if(argc > 2)
          {
            bool print = false;
            for(int i = 2; i < argc; i++)
            {
              for(size_t j = 1; j < MAX_COM; j++)
              {
                if(!strcasecmp(command[j], argv[i]))
                {
                  help(j - 1);
                  print = true;
                  break;
                }
              }
            }
            if(print)
            {
              return 0;
            }
          }
        }
        else if(!strcasecmp(command_.c_str(), command[GET_COM]))
        {
          return get(argc, argv);
        }
        else if(!strcasecmp(command_.c_str(), command[PUT_COM]))
        {
          return put(argc, argv);
        }
      }
      help(MAX_TOPIC);
      return 1;
    }

  }
}

int main(int argc, char* argv[])
{
  AdServer::Controlling::ControlAdmin admin;
  return admin.run(argc, argv);
}

