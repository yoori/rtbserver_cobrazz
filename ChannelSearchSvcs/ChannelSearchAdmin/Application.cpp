
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>

#include <String/StringManip.hpp>
#include <Generics/AppUtils.hpp>

#include <UtilCommons/Table.hpp>

#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "\nUsage:\nChannelSearchAdmin <command> <command arguments>\n\n"
    "Synopsis 1:\n"
    "\tChannelSearchAdmin help\n\n"
    "Synopsis 2:\n"
    "\tChannelSearchAdmin search -r <channel search service object reference>"
    " --phrase=<word list>\n\n"
    "Sample:\n"
    "\tChannelSearchAdmin search -r corbaloc:iiop:dev.ocslab.com:18102/ChannelSearch"
    " --phrase=car\n"
    "\tChannelSearchAdmin match -r corbaloc:iiop:dev.ocslab.com:18102/ChannelSearch"
    " --phrase=car --url=test.com --limit=10\n";

  const Table::Column RESULT_TABLE_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("reuse", Table::Column::NUMBER)
  };

  const Table::Column MATCH_RESULT_TABLE_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("channel_trigger_id", Table::Column::TEXT),
    Table::Column("ccgs", Table::Column::TEXT)
  };

  typedef std::unique_ptr<Table> TablePtr;
}

////////////////////////////////////////////////////
// class Application
////////////////////////////////////////////////////

int
main(int argc, char** argv)
{
  Application* app = 0;

  try
  {
    app = Application::instance();
    return app->run(argc, argv);
  }
  catch (const Application::InvalidArgument& ex)
  {
    std::cerr << "Invalid argument. Exception caught:\n" << ex.what() <<
      "\nRun 'ChannelSearchAdmin help' for usage details\n";
  }
  catch (const Application::Exception& ex)
  {
    std::cerr << "ChannelSearchAdmin: Application::Exception exception caught. "
      ":" << std::endl << ex.what() << std::endl;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "ChannelSearchAdmin: eh::Exception exception caught. "
      ":" << std::endl << ex.what() << std::endl;
  }
  catch (const AdServer::ChannelSearchSvcs::ChannelSearch::
    ImplementationException& ex)
  {
    std::cerr << "ChannelSearchAdmin: ChannelSearch::ImplementationException "
      "exception caught. :" << std::endl << ex.description.in() <<
      std::endl;
  }
  catch (const CORBA::Exception& ex)
  {
    std::cerr << "ChannelSearchAdmin: CORBA::Exception exception caught. "
      ":" << std::endl << ex << std::endl;
  }
  catch (...)
  {
    std::cerr << "ChannelSearchAdmin: unknown exception caught" << std::endl;
  }

  return -1;
}

Application::Application() /*throw(Application::Exception, eh::Exception)*/
{
}

Application::~Application() noexcept
{
  channel_search_ = 0;

  if (!CORBA::is_nil(orb_))
  {
    orb_->destroy();
  }
}

int
Application::run(int& argc, char** argv)
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::StringOption opt_phrase("");
  Generics::AppUtils::StringOption opt_url("");
  Generics::AppUtils::Option<std::string> opt_ref;
  Generics::AppUtils::Option<CORBA::Long> opt_limit(10000);
  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);

  args.add(
    Generics::AppUtils::equal_name("phrase") ||
    Generics::AppUtils::short_name("p"),
    opt_phrase);

  args.add(
    Generics::AppUtils::equal_name("url") ||
    Generics::AppUtils::short_name("u"),
    opt_url);

  args.add(
    Generics::AppUtils::equal_name("ref") ||
    Generics::AppUtils::short_name("r"),
    opt_ref);

  args.add(
    Generics::AppUtils::equal_name("limit") ||
    Generics::AppUtils::short_name("l"),
    opt_limit);

  args.parse(argc - 1, argv + 1);

  orb_ = CORBA::ORB_init(argc, argv);

  if (CORBA::is_nil(orb_))
  {
    throw Exception("CORBA::ORB_init failed");
  }

  if (argc < 2)
  {
    throw InvalidArgument("Too few arguments");
  }

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  std::string cmd = *commands.begin();

  if (cmd == "search" || cmd == "match")
  {
    if(!opt_ref.installed())
    {
      std::cout << "ChannelSearchService reference isn't defined."
        << std::endl
        << USAGE << std::endl;
      return 1;
    }

    {
      std::string service_ref = *opt_ref;
      std::string url = *opt_url;
      std::string phrase = *opt_phrase;

      try
      {
        CORBA::Object_var obj = orb_->string_to_object(service_ref.c_str());

        if (CORBA::is_nil(obj))
        {
          Stream::Error ostr;
          ostr << "string_to_object failed for service reference '" <<
            service_ref << "'";
          throw Exception(ostr);
        }

        channel_search_ =
          AdServer::ChannelSearchSvcs::ChannelSearch::_narrow(obj.in());

        if (CORBA::is_nil(channel_search_))
        {
          Stream::Error ostr;
          ostr << "AdServer::ChannelSearchSvcs::ChannelSearch::_narrow() failed "
            "for service reference '" << service_ref << "'";
          throw Exception(ostr);
        }
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << "failed to resolve service reference '" <<
          service_ref << "'. CORBA::SystemException caught:\n" << ex;
        throw Exception(ostr);
      }

      try
      {
        if(cmd == "search")
        {
          ResultSeq_var result = search(phrase.c_str());
          print_result(result.in());
        }
        else if(cmd == "match")
        {
          AdServer::ChannelSearchSvcs::MatchInfo_var result =
            channel_search_->match(url.c_str(), phrase.c_str(), *opt_limit);
          print_match_result(result);
        }
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << "failed to do search request. "
          "CORBA::SystemException caught:\n" << ex;
        throw Exception(ostr);
      }
      catch (const AdServer::ChannelSearchSvcs::
             ChannelSearch::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << "failed to resolve service reference '"
          << service_ref << "'. ImplementationException caught:\n"
          << ex.description;
        throw Exception(ostr);
      }
    }
  }
  else
  {
    Stream::Error ostr;
    ostr << "Unknown command '" << cmd << "'";
    throw InvalidArgument(ostr);
  }
  return 0;
}

int
Application::help()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  std::cerr << USAGE << std::endl;
  return 0;
}

Application::ResultSeq *
Application::search(const char *phrase)
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  if (!channel_search_)
  {
    return 0;
  }
  return channel_search_->search(phrase);
}

void
Application::print_result(const ResultSeq &result)
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  size_t columns =
    sizeof(RESULT_TABLE_COLUMNS) / sizeof(RESULT_TABLE_COLUMNS[0]);
  TablePtr table(new Table(columns));
  for (size_t i = 0; i < columns; i++)
  {
    table->column(i, RESULT_TABLE_COLUMNS[i]);
  }
  for (size_t i = 0; i < result.length(); i++)
  {
    Table::Row row(table->columns());
    row.add_field(result[i].channel_id);
    row.add_field(result[i].reuse);
    table->add_row(row);
  }
  std::cout << std::endl;
  std::cout << "Number of channels: " << result.length() << std::endl;
  std::cout << std::endl;
  table->dump(std::cout);
}

void
Application::print_match_result(
  const AdServer::ChannelSearchSvcs::MatchInfo& result)
  /*throw(Exception, eh::Exception, CORBA::Exception)*/
{
  {
    size_t columns =
      sizeof(MATCH_RESULT_TABLE_COLUMNS) / sizeof(MATCH_RESULT_TABLE_COLUMNS[0]);
    TablePtr table(new Table(columns));
    for (size_t i = 0; i < columns; i++)
    {
      table->column(i, MATCH_RESULT_TABLE_COLUMNS[i]);
    }
    for (size_t i = 0; i < result.channels.length(); i++)
    {
      Table::Row row(table->columns());
      row.add_field(result.channels[i].channel_id);

      std::ostringstream tr_ostr;
      for(CORBA::ULong tr_i = 0; tr_i < result.channels[i].triggers.length(); ++tr_i)
      {
        if(tr_i != 0)
        {
          tr_ostr << ", ";
        }

        tr_ostr << result.channels[i].triggers[tr_i];
      }

      row.add_field(tr_ostr.str());

      std::ostringstream ccg_ostr;
      for(CORBA::ULong ccg_i = 0; ccg_i < result.channels[i].ccgs.length(); ++ccg_i)
      {
        if(ccg_i != 0)
        {
          ccg_ostr << ", ";
        }

        ccg_ostr << result.channels[i].ccgs[ccg_i];
      }

      row.add_field(ccg_ostr.str());

      table->add_row(row);
    }

    table->dump(std::cout);
  }

  std::cout << std::endl;
  std::cout << "Number of channels: " << result.channels.length() << std::endl;
  std::cout << std::endl;
}
