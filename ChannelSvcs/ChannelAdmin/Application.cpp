
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <unistd.h>

#include <String/StringManip.hpp>
#include <Logger/StreamLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/Network.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Commons/CorbaTypes.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelCommons.hpp>
#include <ChannelSvcs/ChannelCommons/Serialization.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUpdateBase.hpp>

#include <ChannelSvcs/DictionaryProvider/DictionaryProvider.hpp>
#include <UtilCommons/Table.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelServerSessionFactory.hpp>
#include "Application.hpp"

namespace
{
  namespace
  {
    const char* special_names[2]={
      "MR_NO_ADV",
      "MR_NO_TRACK"};
  }

  enum
  {
    command_help  = 0,
    command_match,
    command_update,
    command_check,
    command_all_ccg,
    command_ccg_traits,
    command_lexemes,
    command_max
  };
  const char *COMMAND_TOPICS[] =
  {
    "help",
    "match",
    "update",
    "check",
    "all_ccg",
    "ccg_traits",
    "lexemes",
    "commands"
  };
  const char *HELP[] = 
  {
    "\nUsage:ChannelAdmin command [options]\n"
    "See details: ChannelAdmin help [topic]\n"
    "Topics: match, update, check, all_ccg, ccg_traits lexemes, commands\n",//help
    "\nChannelAdmin match -r[--reference] reference "
      "[-P[--host_and_port] host:port]"
      " [-a[--stat]] {all,norm,nomax,nofirst,*nostat} -t[--times] times "
    "-u[--urls] url -p[--pwords] page_word -e [--swords] search_word -U uid\n"
      "Options:\n"
      "-r[--reference] reference: set CORBA reference\n"
      "-P[--host_and_port] host:port: set host and port for CORBA reference "
      "if it didn't set\n"
      "-a[--stat] calculate time execution statistic of call\n"
      "-t n: do query n times\n"
      "-d[--non-strict-hard] enable non-strict word matching\n"
      "-f[--non-strict-soft] enable non-strict word matching\n"
      "-l[--non-strict-url] enable non-strict-url matching\n"
      "-u[--urls] word: phrase for url matching\n"
      "-U[--uid] uid: uid in text representation\n"
      "-C[--no-print-content] don't print content channels\n"
      "-S[--no-simplify-page] don't simplify page words\n"
      "-p[--pwords] word: phrase for page words matching\n"
      "-e[--swords] word: phrase for search words matching\n"
      "-j[--statuses] statuses of search channels, by default 'A'\n"
      "-b[--negative-trigger]  return negative trigger as positive, "
      "works only for non strict matching\n"
      "-o[--show] set a filter\n"
      "  filter option ::= <field name><rel op><value>\n"
      "  rel op ::= ~ | !~ | = | != | < | > | <= | >=  // '~' relational operator means \"contained\"\n"
      "At least one of u,p,e,o options should present.\n"
      "Examples:\n ./ChannelAdmin match -sr "
      "corbaloc:iiop:localhost:2104/ChannelManagerController "
      " -a norm -t 10 -u dev.ocslab.com -p \"steal money\" -e prison\n"
      "Make matching query to server working on localhost and 2104 port,"
      " do 10 queries and calculate statistic,"
      " ask refer dev.ocslab.com, page word \"steal money\""
      " and search word prison.\n"
      "./ChannelAdmin match -r corbaloc:iiop:localhost:2103/ChannelServer "
      " -u dev.ocslab.com -p \"steal money\" -e prison\n",//match
    "\nChannelAdmin update  -r[--reference] reference "
      "[-P[--host_and_port] host:port] "
      " [-a[--stat]] {all,norm,nomax,nofirst,*nostat} -t[--times] times "
      "-i[--channels] channel_id[,channel_id2,...,channel_idN]\n"
      "Options:\n"
      " -r[--reference] set corba reference\n"
      " -P[--host_and_port] host:port: set host and port for CORBA reference "
      "if it didn't set\n"
      " -a[--stat] calculate time execution statistic of call\n"
      " -t n: do query n times\n"
      " -i[--channels] channel list ids\n"
      " -T[--print-channel-trigger-id]\n"
      "Channel ids and trigger ids break \",\"\n"
      "Examples:\n"
      "./ChannelAdmin update -r corbaloc:iiop:localhost:2103/ChannelUpdate "
      " -i 118,120 \n"
      " Get content of channels and stamp for channels with id 118,120 "
      "from ChannelServer\n"
      "./ChannelAdmin update -r "
      "corbaloc:iiop:localhost:2104/ChannelManagerController  -i 118,120 \n"
      " Get content of channels and stamp for channels with id 118,120. "
      "from ChannelController\n",//update
    "\nChannelAdmin check -r[--reference] "
    "[-P[--host_and_port] host:port] "
    " [-a[--stat]] {all,norm,nomax,nofirst,*nostat} -t[--times] times "
    "[-g]--go-from-date master_time stamp -v[--version] version "
    "-s[--use_only_list] "
    "-i[--channels] channel_id1,channel_id2,...,channel_idN] "
    "-n[--colo] colo_id -o[--show] field -q[--equal] value\n"
    "Options:\n"
      " -P[--host_and_port] host:port: set host and port for CORBA reference "
      "if it didn't set\n"
      " -a[--stat] calculate time execution statistic of call\n"
      " -t n: do query n times\n"
      " -v[--version] version of ChannelServer, set it, only"
      "if you really know that you do\n"
      " -h[--special] show special channels\n"
      " -H[--only-special] show only special channels\n"
      " -n[--colo] colo id\n"
      " -g[--fo-from-date] master time stamp\n"
      " -i[--channels] channel list ids for getting their information\n"
      " -s[--use_only_list] use id only from list\n"
      " -o[--show] set a filter\n"
      "  filter option ::= <field name><rel op><value>\n"
      "  rel op ::= ~ | !~ | = | != | < | > | <= | >=  // '~' relational operator means \"contained\"\n"
      "For successful executing should be set reference to channel controller"
      " or channel server. Call returns list of channel list id with stamps "
      "more than master time stamp. If master time stamp didn't set in "
      "arguments, the 1970-01-01-00:00:00 is used as master time stamp\n"
      "Example:\n"
      "ChannelAdmin check -r corbaloc:iiop:localhost:10003/ChannelUpdate"
      " -hg 2008-10-09-12:57:59\n" "Print channel list ids with stamp more than"
      " 2008-10-09-12:57:59 and special channels\n", //check
    "\nChannelAdmin all_ccg -r[--reference] ref "
      "[-P[--host_and_port] host:port] "
      "[-a[--stat]] {all,norm,nomax,nofirst,*nostat} -t[--times] times "
      "-g[--go-from-date] master_date " "-m[--limit] max_ccg "
      "-n[--first] first_channel_id "
      "-i[--channels] channel_id1,channel_id2,...,channel_idN] "
      "-s[--use_only_list] "
      "-o[--show] field -q[--equal] value\n"
    "Options:\n -r set corba reference\n"
      " -P[--host_and_port] host:port: set host and port for CORBA reference "
      "if it didn't set\n"
      " -a[--stat] calculate time execution statistic of call\n"
      " -t n: do query n times\n"
      " -g[--fo-from-date] master time stamp, "
      "by default is 1970-01-01-00:00:00\n"
      " -m[--limit] maximum ids in answer\n"
      " -n[--first] first channel id, chanenl_id should be not less "
      "than this value, by default is 0\n"
      " -i[--channels] channel list ids for forced update\n"
      " -s[--use_only_list] use id only from list\n"
      "-o[--show] set a filter\n"
      "  filter option ::= <field name><rel op><value>\n"
      "  rel op ::= ~ | !~ | = | != | < | > | <= | >=  // '~' relational operator means \"contained\"\n"
      "For successful executing should be set reference to channel controller"
      " or channel server. Call returns table of positive ccg."
      "Stamp should be more than master time stamp. "
      "If master time stamp didn't set in arguments, "
      "the current time is used as master time stamp.\n"
      "Example:\n"
      "ChannelAdmin all_ccg -r corbaloc:iiop:localhost:2103/ChannelUpdate "
      "-g 2008-10-09-12:57:59 -n 1000 -m 10\n",
    "\nChannelAdmin ccg_traits -r[--reference] ref "
        "[-P[--host_and_port] host:port] "
        "[-a[--stat]] {all,norm,nomax,nofirst,*nostat} -t[--times] times "
        "-i[--channels] channel_id1[,channel_id2,...,channnel_idN]\n"
    "Options:\n -r set corba reference\n"
      " -P[--host_and_port] host:port: set host and port for CORBA reference "
      "if it didn't set\n"
      " -a[--stat] calculate time execution statistic of call\n"
      " -t n: do query n times\n"
      " -i[--channels] set channel ids\n"
      "Channel ids break by \',\'\n"
      "Example:\n"
      "ChannelAdmin ccg_traits -r corbaloc:iiop:localhost:2103/ChannelUpdate -i 1034,18\n",
    "\nChannelAdmin lexemes r[--reference] ref "
        "[-P[--host_and_port] host:port] "
        "-L[--lexemes] lexeme\n"
    "Options:\n -r set corba reference to dictionary service\n"
      " -P[--host_and_port] host:port: set host and port for CORBA reference "
      "if it didn't set\n"
      " -L[--lexemes] lexeme\n"
      " -A[--lang] language\n"
      "Example:\n"
      "ChannelAdmin lexemes -r corbaloc:iiop:localhost:2110/DictionaryProvider -L мама -L мыла -L раму -A ru\n",
    "\nCommnds: match update check all_ccg ccg_traits lexemes help\n"//commands
  };
  const char *COMMAND[command_max] =
  {
    "help",
    "match",
    "update",
    "check",
    "all_ccg",
    "ccg_traits",
    "lexemes"
  };

  Table::Column CCG_KEYWORDS_FIELDS[] =
  {
    Table::Column("ccg_keyword_id", Table::Column::NUMBER),
    Table::Column("ccg_id", Table::Column::NUMBER),
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("max_cpc"),
    Table::Column("ctr"),
    Table::Column("click_url"),
    Table::Column("original_keyword")
  };

  Table::Column UNMATCHED_FIELDS[] =
  {
    Table::Column("trigger"),
    Table::Column("type")
  };

  Table::Column MATCH_QUERY_FIELDS[] =
  {
    Table::Column("query"),
    Table::Column("host_name"),
    Table::Column("query_date"),
    Table::Column("reference"),
    Table::Column("ns_words"),
    Table::Column("ns_url"),
    Table::Column("page"),
    Table::Column("search"),
    Table::Column("urls"),
    Table::Column("uid")
  };

  const size_t count_match_query_fields =
    sizeof(MATCH_QUERY_FIELDS) / sizeof(MATCH_QUERY_FIELDS[0]);


  Table::Column CHECK_QUERY_FIELDS[] =
  {
    Table::Column("query"),
    Table::Column("host_name"),
    Table::Column("query_date"),
    Table::Column("reference"),
    Table::Column("master"),
    Table::Column("use_only_list"),
    Table::Column("channel_ids")
  };

  const size_t count_check_query_fields =
    sizeof(CHECK_QUERY_FIELDS) / sizeof(CHECK_QUERY_FIELDS[0]);

  Table::Column UPDATE_QUERY_FIELDS[] =
  {
    Table::Column("query"),
    Table::Column("host_name"),
    Table::Column("query_date"),
    Table::Column("reference"),
    Table::Column("trigger_ids")
  };

  const size_t count_update_query_fields =
    sizeof(UPDATE_QUERY_FIELDS)/sizeof(UPDATE_QUERY_FIELDS[0]);

  Table::Column UPDATE_FIELDS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("url"),
    Table::Column("neg_url"),
    Table::Column("url_keyword"),
    Table::Column("neg_url_keyword"),
    Table::Column("page_word"),
    Table::Column("neg_page_word"),
    Table::Column("search_word"),
    Table::Column("neg_search_word"),
    Table::Column("uids"),
    Table::Column("stamp")
  };

  const size_t count_update_fields =
    sizeof(UPDATE_FIELDS)/sizeof(UPDATE_FIELDS[0]);


  Table::Column CCG_QUERY_FIELDS[] =
  {
    Table::Column("query"),
    Table::Column("host_name"),
    Table::Column("query_date"),
    Table::Column("reference"),
    Table::Column("master"),
    Table::Column("start_id", Table::Column::NUMBER),
    Table::Column("limit", Table::Column::NUMBER),
    Table::Column("use_only_list")
  };

  const size_t count_ccg_query_fields =
    sizeof(CCG_QUERY_FIELDS)/sizeof(CCG_QUERY_FIELDS[0]);

  Table::Column CCG_TRAITS_QUERY_FIELDS[] =
  {
    Table::Column("query"),
    Table::Column("host_name"),
    Table::Column("query_date"),
    Table::Column("reference"),
    Table::Column("channels")
  };
  const size_t count_ccg_traits_query_fields =
    sizeof(CCG_TRAITS_QUERY_FIELDS)/sizeof(CCG_TRAITS_QUERY_FIELDS[0]);

  Table::Column LEXEMES_QUERY_FIELDS[] =
  {
    Table::Column("query"),
    Table::Column("host_name"),
    Table::Column("query_date"),
    Table::Column("reference"),
    Table::Column("language"),
    Table::Column("lexemes")
  };
  const size_t count_lexemes_query_fields =
    sizeof(LEXEMES_QUERY_FIELDS)/sizeof(LEXEMES_QUERY_FIELDS[0]);

  Table::Column LEXEMES_FIELDS[] =
  {
    Table::Column("lexeme"),
    Table::Column("forms")
  };
  const size_t count_lexemes_fields =
    sizeof(LEXEMES_FIELDS)/sizeof(LEXEMES_FIELDS[0]);

  template<class T>
  struct Option
  {
    typedef T type_name;
    const unsigned long id;
    const char* equal_name;
    const char* short_name;
    T value;
  };

  enum CheckOptionId
  {
    OPT_SPECIAL,
    OPT_ONLY_SPECIAL,
    OPT_NS_WORD_H,
    OPT_NS_WORD_S,
    OPT_NS_WORD_U,
    OPT_NEGATIVE,
    OPT_PRINT_CHT_ID,
    OPT_NO_PRINT_CONTENT,
    OPT_NO_SIMPLIFY_PAGE,
    OPT_ONLY_LIST,
    OPT_WILD_INPUT,
    OPT_CHECK_MAX
  };

  static Option<Generics::AppUtils::CheckOption> check_options[] =
  {
    { OPT_SPECIAL, "special", "h" ,
      Generics::AppUtils::CheckOption() },
    { OPT_ONLY_SPECIAL, "only-special", "H",
      Generics::AppUtils::CheckOption() },
    { OPT_NS_WORD_H, "non_strict_hard", "d",
      Generics::AppUtils::CheckOption() },
    { OPT_NS_WORD_S, "non_strict_soft", "f",
      Generics::AppUtils::CheckOption() },
    { OPT_NS_WORD_U, "non_strict_url", "l",
      Generics::AppUtils::CheckOption() },
    { OPT_NEGATIVE, "negative", "b",
      Generics::AppUtils::CheckOption() },
    { OPT_PRINT_CHT_ID, "print-channel-trigger-id", "T",
      Generics::AppUtils::CheckOption() },
    { OPT_NO_PRINT_CONTENT, "no-print-content", "C",
      Generics::AppUtils::CheckOption() },
    { OPT_NO_SIMPLIFY_PAGE, "no-simplify-page", "S",
      Generics::AppUtils::CheckOption() },
    { OPT_ONLY_LIST, "only-list", "s",
      Generics::AppUtils::CheckOption() },
    { OPT_WILD_INPUT, "wild-input", "w",
      Generics::AppUtils::CheckOption() }
  };

  enum StringOptionId
  {
    OPT_IDS = 0,
    OPT_REFERENCE,
    OPT_URL,
    OPT_UID,
    OPT_PWORDS,
    OPT_SWORDS,
    OPT_STATUS,
    OPT_DATE,
    OPT_FILTER,
    OPT_HOST_AND_PORT,
    OPT_LANG,
    OPT_STAT,
    OPT_STRING_MAX
  };

  static Option<Generics::AppUtils::StringOption > string_options[] = 
  {
    { OPT_IDS, "channels", "i", Generics::AppUtils::StringOption() },
    { OPT_REFERENCE, "reference", "r", Generics::AppUtils::StringOption() },
    { OPT_URL, "urls", "u", Generics::AppUtils::StringOption() },
    { OPT_UID, "uid", "U", Generics::AppUtils::StringOption() },
    { OPT_PWORDS, "pwords", "p", Generics::AppUtils::StringOption() },
    { OPT_SWORDS, "swords", "e", Generics::AppUtils::StringOption() },
    { OPT_STATUS, "statuses", "j", Generics::AppUtils::StringOption("A") },
    { OPT_DATE, "go-from-date", "g", Generics::AppUtils::StringOption() },
    { OPT_FILTER, "show", "o", Generics::AppUtils::StringOption() },
    { OPT_HOST_AND_PORT, "host_and_port", "P", Generics::AppUtils::StringOption() },
    { OPT_LANG, "lang", "A", Generics::AppUtils::StringOption() },
    { OPT_STAT, "stat", "a", Generics::AppUtils::StringOption() }
  };

  enum ULongOptionId
  {
    OPT_FIRST = 0,
    OPT_LIMIT,
    OPT_TIMES,
    OPT_ULONG_MAX
  };

  static Option<Generics::AppUtils::Option<unsigned long> > ulong_options[] = 
  {
    { OPT_FIRST, "first", "n", Generics::AppUtils::Option<unsigned long>(1) },
    { OPT_LIMIT, "limit", "m",  Generics::AppUtils::Option<unsigned long>(10000) },
    { OPT_TIMES, "times", "t", Generics::AppUtils::Option<unsigned long>(1) }
  };
}

int main(int argc, char** argv)
{
  int ret_value = -100;
  
  try
  {
    Application& app = Application::instance();
    ret_value = app.run(argc, argv);
    app.deactivate_objects();
  }
  catch(const Application::InvalidArgument& e)
  {
    std::cerr << "Invalid argument. Exception caught:\n"
              << e.what() << "\nRun 'ChannelAdmin help' for usage details\n";
  }
  catch(const eh::Exception& e)
  {
    std::cerr << "ChannelAdmin: eh::Exception exception caught. "
      ":" << e.what() << std::endl;    
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "ChannelAdmin: eh::Exception exception caught. "
      ": " << e << std::endl;    
  }
  catch(...)
  {
    std::cerr << "ChannelAdmin: unknown exception caught" << std::endl;
  }
  return ret_value;
}

Application::Application() /*throw(Application::Exception, eh::Exception)*/:
  use_session_(false),
  date_(0)
{
  logger_ = 
    new Logging::OStream::Logger(Logging::OStream::Config(std::cout));
  adapter_ = new CORBACommons::CorbaClientAdapter;
}

Application::~Application() noexcept
{
}

void Application::init_update_interface_() /*throw(InvalidArgument)*/
{
  bool initted = false;
  try
  {
    if (string_options[OPT_REFERENCE].value.installed())
    {
      reference_ = *string_options[OPT_REFERENCE].value;
    }
    else
    {
      reference_ = "corbaloc:iiop:";
      reference_ += *string_options[OPT_HOST_AND_PORT].value;
      reference_ += "/ChannelManagerController";
    }
    obj_ref_ = adapter_->resolve_object(reference_.c_str());

    if (CORBA::is_nil(obj_ref_))
    {
      Stream::Error ostr;
      ostr << "reference is null";
      throw InvalidArgument(ostr);
    }

    AdServer::ChannelSvcs::ChannelManagerController_var manager =
      AdServer::ChannelSvcs::ChannelManagerController::_narrow(obj_ref_.in());
    if(CORBA::is_nil(manager.in()))
    {
      throw InvalidArgument("_narrow return nil reference"); 
    }
    Generics::ActiveObjectCallback_var callback(
      new Logging::ActiveObjectCallbackImpl(logger_.in()));
    AdServer::ChannelSvcs::ChannelServerSessionFactory::init(
        *adapter_,
        0,
        &load_session_factory_,
        callback,
        logger_.in());
    load_session_ = manager->get_load_session();
    if(CORBA::is_nil(load_session_))
    {
      Stream::Error ostr;
      ostr << "load session is nil ";
      throw InvalidArgument(ostr);
    }
    initted = true;
    use_session_ = true;
  }
  catch(const CORBA::SystemException&)
  {
  }
  catch(AdServer::ChannelSvcs::ImplementationException&)
  {
  }
  catch(const CORBACommons::CorbaClientAdapter::Exception&)
  {
  }
  catch(const InvalidArgument&)
  {
  }
  if(!initted)
  {
    try
    {
      if (!string_options[OPT_REFERENCE].value.installed())
      {
        reference_ = "corbaloc:iiop:";
        reference_ += *string_options[OPT_HOST_AND_PORT].value;
        reference_ += "/ChannelUpdate";
        obj_ref_ = adapter_->resolve_object(reference_.c_str());

        if (CORBA::is_nil(obj_ref_))
        {
          Stream::Error ostr;
          ostr << "reference is null";
          throw InvalidArgument(ostr);
        }
      }
      channel_update_ =
        AdServer::ChannelSvcs::ChannelUpdate_v33::_narrow(obj_ref_.in());

      if(CORBA::is_nil(channel_update_))
      {
        throw InvalidArgument("Application::init_update_interface_: "
            "AdServer::ChannelSvcs::ChannelUpdate_v33::_narrow failed ");
      }
    }
    catch(const CORBACommons::CorbaClientAdapter::Exception& e)
    {
      Stream::Error ostr;
      ostr << "failed to resolve channel update reference '" << reference_;
      ostr << "'. CorbaClientAdapter::Exception: " << e.what();
      throw InvalidArgument(ostr);
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << "failed to resolve channel update reference "
        "'. CORBA::SystemException: " << e;
      throw InvalidArgument(ostr);
    }
  }
}

void Application::init_server_interface_() /*throw(InvalidArgument)*/
{
  bool initted = false;
  try
  {
    if (string_options[OPT_REFERENCE].value.installed())
    {
      reference_ = *string_options[OPT_REFERENCE].value;
    }
    else
    {
      reference_ = "corbaloc:iiop:";
      reference_ += *string_options[OPT_HOST_AND_PORT].value;
      reference_ += "/ChannelManagerController";
    }
    obj_ref_ = adapter_->resolve_object(reference_.c_str());

    if (CORBA::is_nil(obj_ref_))
    {
      Stream::Error ostr;
      ostr << "reference is null";
      throw InvalidArgument(ostr);
    }

    AdServer::ChannelSvcs::ChannelManagerController_var manager =
      AdServer::ChannelSvcs::ChannelManagerController::_narrow(obj_ref_.in());

    if(CORBA::is_nil(manager.in()))
    {
      throw InvalidArgument("_narrow return nil reference"); 
    }
    Generics::ActiveObjectCallback_var callback(
      new Logging::ActiveObjectCallbackImpl(logger_.in()));
    AdServer::ChannelSvcs::ChannelServerSessionFactory::init(
        *adapter_,
        &server_session_factory_,
        0,
        callback,
        logger_.in());
    channel_session_ = manager->get_channel_session();
    if(CORBA::is_nil(channel_session_.in()))
    {
      Stream::Error ostr;
      ostr << "channel session is nil";
      throw InvalidArgument(ostr);
    }
    initted = true;
    use_session_ = true;
  }
  catch(const CORBA::SystemException&)
  {
  }
  catch(AdServer::ChannelSvcs::ImplementationException& e)
  {
  }
  catch(const CORBACommons::CorbaClientAdapter::Exception&)
  {
  }
  catch(const InvalidArgument&)
  {
  }
  if(!initted)
  {
    try
    {
      if (!string_options[OPT_REFERENCE].value.installed())
      {
        reference_ = "corbaloc:iiop:";
        reference_ += *string_options[OPT_HOST_AND_PORT].value;
        reference_ += "/ChannelServer";
        obj_ref_ = adapter_->resolve_object(reference_.c_str());

        if (CORBA::is_nil(obj_ref_))
        {
          Stream::Error ostr;
          ostr << "reference is null";
          throw InvalidArgument(ostr);
        }
      }
      channel_server_ =
        AdServer::ChannelSvcs::ChannelServer::_narrow(obj_ref_.in());

      if(CORBA::is_nil(channel_server_))
      {
        throw InvalidArgument("ChannelServer::_narrow failed ");
      }
    }
    catch(const CORBACommons::CorbaClientAdapter::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Failed to resolve channel server reference." 
        " CorbaClientAdapter::Exception: " << e.what();
      throw InvalidArgument(ostr);
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << "Failed to resolve channel server reference." 
        " CORBA::SystemException: " << e;
      throw InvalidArgument(ostr);
    }
  }
}

void Application::parse_ids_(
    const String::SubString& str,
    std::vector<unsigned long>& out)
    /*throw(eh::Exception)*/
{
  String::StringManip::SplitComma tokenizer(str);
  String::SubString token;
  while (tokenizer.get_token(token))
  {
    unsigned long number;
    String::StringManip::str_to_int(token, number);
    out.push_back(number);
  }
  std::sort(out.begin(), out.end());
}

int Application::parse_args_(int argc, char** argv) /*throw(InvalidArgument)*/
{
  try
  {
    Generics::AppUtils::Args args(2);
    for (size_t i = 0; i < OPT_CHECK_MAX; i++)
    {
      args.add(
        Generics::AppUtils::equal_name(check_options[i].equal_name) ||
        Generics::AppUtils::short_name(check_options[i].short_name),
        check_options[i].value);
    }
    for (size_t i = 0; i < OPT_STRING_MAX; i++)
    {
      args.add(
        Generics::AppUtils::equal_name(string_options[i].equal_name) ||
        Generics::AppUtils::short_name(string_options[i].short_name),
        string_options[i].value);
    }
    for (size_t i = 0; i < OPT_ULONG_MAX; i++)
    {
      args.add(
        Generics::AppUtils::equal_name(ulong_options[i].equal_name) ||
        Generics::AppUtils::short_name(ulong_options[i].short_name),
        ulong_options[i].value);
    }
    Generics::AppUtils::OptionsSet<std::vector<std::string> > lex_option;
    args.add(
      Generics::AppUtils::equal_name("lexemes") ||
      Generics::AppUtils::short_name("L"),
      lex_option);
    args.parse(argc - 1, argv + 1);
    lexemes_data_ = *lex_option;
    const Generics::AppUtils::Args::CommandList& commands = args.commands();
    int c_index = command_help;
    if (!commands.empty())
    {
      Generics::AppUtils::Args::CommandList::const_iterator com_it =
        args.commands().begin();
      const std::string& com = *com_it;;
      for (size_t i = 0; i < command_max; i++)
      {
        if (com == COMMAND[i])
        {
          c_index = i;
          break;
        }
      }
      if(++com_it != args.commands().end())
      {
        topic_ = *com_it;
      }
    }
    if (string_options[OPT_IDS].value.installed())
    {
      parse_ids_(*string_options[OPT_IDS].value, channels_id_);
    }
    if (c_index != command_help &&
        !string_options[OPT_REFERENCE].value.installed() &&
        !string_options[OPT_HOST_AND_PORT].value.installed())
    {
      throw InvalidArgument("CORBA reference doesn't set");
    }
    if (c_index == command_match)
    {
      if (!(string_options[OPT_URL].value.installed() ||
          string_options[OPT_UID].value.installed() ||
          string_options[OPT_PWORDS].value.installed() ||
          string_options[OPT_SWORDS].value.installed()))
      {
        throw InvalidArgument("Should be at least one -[Uupe] of options");
      }
    }
    if (string_options[OPT_DATE].value.installed())
    {
      try
      {
        date_.set(*string_options[OPT_DATE].value, "%Y-%m-%d-%H:%M:%S");
      }
      catch(const Generics::Time::InvalidArgument& e)
      {
        Stream::Error err;
        err << "wrong date format: " << e.what();
        throw InvalidArgument(err);
      }
      catch(const Generics::Time::Exception& e)
      {
        Stream::Error err;
        err << "error in date format: " << e.what();
        throw InvalidArgument(err);
      }
    }
    if (string_options[OPT_FILTER].value.installed())
    {
      std::string name, value;
      Table::Relation rel;
      if (Table::parse_filter(
          string_options[OPT_FILTER].value->c_str(), name, rel, value))
      {
        filters_.push_back(Table::Filter(name.c_str(), value.c_str(), rel));
      }
    }
    return c_index;
  }
  catch(const eh::Exception& e)
  {
    throw InvalidArgument(e.what());
  }
}

int
Application::run(int argc, char** argv)
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::SystemException)*/
{
  int c_index = parse_args_(argc, argv);
  if(c_index == command_help)
  {
    return help(topic_);
  }

  switch(c_index)
  {
    case command_match:
      return match_();
      break;
    case command_check:
      return smartcheck_();
      break;
    case command_update:
      return update_();
      break;
    case command_all_ccg:
      return pos_ccg_();
      break;
    case command_ccg_traits:
      return ccg_traits_();
      break;
    case command_lexemes:
      return lexemes_();
      break;
    default:
      break;
  }
  return 0;
}

void Application::deactivate_objects()
  /*throw(Generics::CompositeActiveObject::Exception, eh::Exception)*/
{
  if(server_session_factory_.in())
  {
    server_session_factory_->deactivate_object();
    server_session_factory_->wait_object();
  }
  if(load_session_factory_.in())
  {
    load_session_factory_->deactivate_object();
    load_session_factory_->wait_object();
  }
}

int
Application::help(const std::string& topic) const 
  noexcept
{
  size_t i = 0;
  for (i = 0; i < sizeof(COMMAND_TOPICS) / sizeof(COMMAND_TOPICS[0]) - 1; i++)
  {
    if(topic == COMMAND_TOPICS[i])
    {
      break;
    }
  }
  std::cerr << HELP[i];
  return 0;
}

void Application::add_query_header_(
  const char* query,
  const char* reference,
  Table::Row& row) noexcept
{
  char buf[4096];
  row.add_field(query);
  if(gethostname(buf, sizeof(buf)) == 0)
  {
    row.add_field(buf);
  }
  else
  {
    row.add_field("localhost");
  }
  row.add_field(Generics::Time::get_time_of_day().get_gm_time().format(
      "%Y-%m-%d-%H:%M:%S"));
  row.add_field(reference);
}

int Application::smartcheck_()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::SystemException)*/
{
  init_update_interface_();
  AdServer::ChannelSvcs::ChannelCurrent::CheckQuery query;
  AdServer::ChannelSvcs::ChannelCurrent::CheckData_var result;
  query.colo_id = 1;
  query.version = "";
  query.master_stamp = CorbaAlgs::pack_time(date_);
  query.use_only_list = check_options[OPT_ONLY_LIST].value.enabled();
  query.new_ids.length(channels_id_.size());
  std::copy(
    channels_id_.begin(),
    channels_id_.end(),
    query.new_ids.get_buffer());

  StatMarker stat_marker(
    *string_options[OPT_STAT].value,
    "Check",
    *ulong_options[OPT_TIMES].value);
  try
  {
    if (use_session_)
    {
      typedef decltype(&AdServer::ChannelSvcs::ChannelLoadSession::check) FuncType;
      FuncType func_ptr = &AdServer::ChannelSvcs::ChannelLoadSession::check;
      stat_marker.calc_stat(&*load_session_, func_ptr, query, result);
    }
    else
    {
      typedef decltype(&AdServer::ChannelSvcs::ChannelUpdateBase_v33::check) FuncType;
      FuncType func_ptr = &AdServer::ChannelSvcs::ChannelUpdateBase_v33::check;
      stat_marker.calc_stat(&*channel_update_, func_ptr, query, result);
    }
  }
  catch(const AdServer::ChannelSvcs::ImplementationException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": ImplementationException: " << e.description;
    throw Exception(ostr);
  }
  catch(const AdServer::ChannelSvcs::NotConfigured& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": NotConfigured: " << e.description;
    throw Exception(ostr);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": CORBA::SystemException: " << e;
    throw Exception(ostr);
  }
  std::unique_ptr<Table> table(new Table(count_check_query_fields));
  for(size_t i = 0; i < count_check_query_fields; i++)
  {
    table->column(i, CHECK_QUERY_FIELDS[i]);
  }
  Table::Row row_q(count_check_query_fields);
  add_query_header_("check", reference_.c_str(), row_q);
  row_q.add_field(date_.get_gm_time().format("%Y-%m-%d-%H:%M:%S"));
  row_q.add_field(check_options[OPT_ONLY_LIST].value.enabled() ? "yes" : "no");
  row_q.add_field(concat_sequence(channels_id_.begin(), channels_id_.end()));
  table->add_row(row_q);
  table->dump(std::cout);
  date_ = CorbaAlgs::unpack_time(result->master_stamp);
  Generics::Time max_time = CorbaAlgs::unpack_time(result->max_time);
  {
    table.reset(new Table(4));
    Table::Row row(4);
    table->column(0, Table::Column("first_stamp"));
    table->column(1, Table::Column("master_stamp"));
    table->column(2, Table::Column("longest_update"));
    table->column(3, Table::Column("source_id"));

    Generics::Time first_date = CorbaAlgs::unpack_time(result->first_stamp);
    row.add_field(first_date.get_gm_time().format("%Y-%m-%d-%H:%M:%S"));
    row.add_field(date_.get_gm_time().format("%Y-%m-%d-%H:%M:%S"));
    row.add_field(max_time.tv_sec);
    row.add_field(result->source_id);
    table->add_row(row);
    table->dump(std::cout);
    if(check_options[OPT_SPECIAL].value.enabled() ||
       check_options[OPT_ONLY_SPECIAL].value.enabled())
    {
      table.reset(new Table(2));
      Table::Row row(2);
      for(size_t i = 0; i < 2;i++)
      {
        table->column(i, Table::Column(special_names[i]));
      }
      if(result->special_adv)
      {
        row.add_field("+");
      }
      else
      {
        row.add_field("-");
      }
      if(result->special_track)
      {
        row.add_field("+");
      }
      else
      {
        row.add_field("-");
      }
      table->add_row(row);
      table->dump(std::cout);
    }
  }
  if(!check_options[OPT_ONLY_SPECIAL].value.enabled() && result->versions.length() > 0)
  {
    std::unique_ptr<Table> table(new Table(3));
    Table::Row row(3);
    table->column(0, Table::Column("id", Table::Column::NUMBER));
    table->column(1, "size");
    table->column(2, "version");
    for(size_t i=0;i<result->versions.length();i++)
    {
      AdServer::ChannelSvcs::ChannelCurrent::ChannelVersion& 
        cur = result->versions[i];
      row.clear();
      row.add_field(cur.id);
      row.add_field(cur.size);
      row.add_field(
        CorbaAlgs::unpack_time(cur.stamp).get_gm_time().format(
          "%Y-%m-%d-%H:%M:%S"));
      table->add_row(row, filters_);
    }
    table->dump(std::cout);
  }
  return 0;
}

template<typename CORBATYPE>
void Application::add_triggers(
  const CORBATYPE& data,
  std::ostream& positive_page_stream,
  std::ostream& negative_page_stream,
  std::ostream& positive_search_stream,
  std::ostream& negative_search_stream,
  std::ostream& positive_url_keyword_stream,
  std::ostream& negative_url_keyword_stream,
  std::ostream& positive_url_stream,
  std::ostream& negative_url_stream,
  std::ostream& uid_stream,
  bool print_cht_id)
  /*throw(eh::Exception)*/
{
  typedef std::vector<unsigned long> IdsVector;
  typedef std::map<std::string, IdsVector> TriggerMap;
  if(data.length())
  {
    TriggerMap maps[9];
    std::ostream* cur_stream = 0;
    size_t index, j = 0;
    do
    {
      index = 0;
      switch(AdServer::ChannelSvcs::Serialization::trigger_type(
          data[j].trigger.get_buffer()))
      {
        case 'P':
          break;
        case 'S':
          index = 2;
          break;
        case 'U':
          index = 4;
          break;
        case 'R':
          index = 6;
          break;
        case 'D':
          index = 8;
          break;
      }
      if(AdServer::ChannelSvcs::Serialization::negative(
          data[j].trigger.get_buffer()))
      {
        index++;
      }
      std::string trigger;
      AdServer::ChannelSvcs::Serialization::get_trigger(
        data[j].trigger.get_buffer(),
        data[j].trigger.length(),
        trigger);
      maps[index][trigger].push_back(data[j].channel_trigger_id);
    } while(++j < data.length());
    for(j = 0; j < sizeof(maps)/sizeof(maps[0]); j++)
    {
      switch(j)
      {
        case 0:
          cur_stream = &positive_page_stream;
          break;
        case 1:
          cur_stream = &negative_page_stream;
          break;
        case 2:
          cur_stream = &positive_search_stream;
          break;
        case 3:
          cur_stream = &negative_search_stream;
          break;
        case 4:
          cur_stream = &positive_url_stream;
          break;
        case 5:
          cur_stream = &negative_url_stream;
          break;
        case 6:
          cur_stream = &positive_url_keyword_stream;
          break;
        case 7:
          cur_stream = &negative_url_keyword_stream;
          break;
        default:
          cur_stream = &uid_stream;
          break;
      }
      const TriggerMap& map = maps[j];
      for(TriggerMap::const_iterator map_it = map.begin();
          map_it != map.end(); ++map_it)
      {
        if(map_it != map.begin())
        {
          *cur_stream << ',';
        }
        *cur_stream << map_it->first;
        if(print_cht_id)
        {
          const IdsVector& vec = map_it->second;
          *cur_stream << '(';
          for(IdsVector::const_iterator vec_it = vec.begin();
              vec_it != vec.end(); ++vec_it)
          {
            if(vec_it != vec.begin())
            {
              *cur_stream << ',';
            }
            *cur_stream << *vec_it;
          }
          *cur_stream << ')';
        }
      }
    }
  }
}

int Application::update_()
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::SystemException)*/
{
  init_update_interface_();
  if(channels_id_.empty())
  {
    throw InvalidArgument(
      "Should be at least one trigger id "
      "or channel id and CampaignServer reference "
      "see -i or -c");
  }

  ::AdServer::ChannelSvcs::ChannelIdSeq ids;
  ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_var result;
  ids.length(channels_id_.size());
  std::unique_ptr<Table> table(new Table(count_update_query_fields));
  {
    Table::Row row(count_update_query_fields);
    for(size_t i = 0; i < count_update_query_fields; i++)
    {
      table->column(i, UPDATE_QUERY_FIELDS[i]);
    }
    add_query_header_("update", reference_.c_str(), row);
    std::ostringstream ostr;
    size_t i = 0;
    do
    {
      ids[i] = channels_id_[i];
      if(i != 0)
      {
        ostr << ", ";
      }
      ostr << channels_id_[i];
    }while(++i < channels_id_.size());
    row.add_field(ostr.str());
    table->add_row(row);
    table->dump(std::cout);
  }
  StatMarker stat_marker(
    *string_options[OPT_STAT].value,
    "Update",
    *ulong_options[OPT_TIMES].value);
  try
  {
    if (use_session_)
    {
      typedef
        decltype(&AdServer::ChannelSvcs::ChannelLoadSession::update_triggers)
        FuncType;
      FuncType func_ptr =
        &AdServer::ChannelSvcs::ChannelLoadSession::update_triggers;
      stat_marker.calc_stat(&*load_session_, func_ptr, ids, result);
    }
    else
    {
      typedef
        decltype(&AdServer::ChannelSvcs::ChannelUpdateBase_v33::update_triggers)
        FuncType;
      FuncType func_ptr =
        &AdServer::ChannelSvcs::ChannelUpdateBase_v33::update_triggers;
      stat_marker.calc_stat(&*channel_update_, func_ptr, ids, result);
    }
  }
  catch(const AdServer::ChannelSvcs::ImplementationException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": ImplementationException: " << e.description;
    throw Exception(ostr);
  }
  catch(const AdServer::ChannelSvcs::NotConfigured& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": NotConfigured: " << e.description;
    throw Exception(ostr);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": CORBA::SystemException: " << e;
    throw Exception(ostr);
  }

  table.reset(new Table(1));
  table->column(0, Table::Column("source_id", Table::Column::NUMBER));
  {
    Table::Row row(1);
    row.add_field(result->source_id);
    table->add_row(row);
  }
  table->dump(std::cout);
  table.reset(new Table(count_update_fields));
  for(size_t i = 0; i < count_update_fields; i++)
  {
    table->column(i, UPDATE_FIELDS[i]);
  }
  for(size_t i = 0; i < result->channels.length(); i++)
  {
    std::ostringstream url, neg_url;
    std::ostringstream url_keyword, neg_url_keyword;
    std::ostringstream page_word, neg_page_word;
    std::ostringstream search_word, neg_search_word;
    std::ostringstream uid;
    add_triggers(
      result->channels[i].words,
      page_word,
      neg_page_word,
      search_word,
      neg_search_word,
      url_keyword,
      neg_url_keyword,
      url,
      neg_url,
      uid,
      check_options[OPT_PRINT_CHT_ID].value.enabled());
    Generics::Time stamp_time = CorbaAlgs::unpack_time(result->channels[i].stamp);
    Table::Row row(table->columns());
    row.add_field(result->channels[i].channel_id);
    row.add_field(url.str());
    row.add_field(neg_url.str());
    row.add_field(url_keyword.str());
    row.add_field(neg_url_keyword.str());
    row.add_field(page_word.str());
    row.add_field(neg_page_word.str());
    row.add_field(search_word.str());
    row.add_field(neg_search_word.str());
    row.add_field(uid.str());
    row.add_field(stamp_time.get_gm_time().format("%Y-%m-%d-%H:%M:%S.%q"));
    table->add_row(row);
  }
  table->dump(std::cout);
  return 0;
}

void Application::print_ccg_query_params_() noexcept
{
  std::unique_ptr<Table> table(new Table(count_ccg_query_fields));
  for(size_t i = 0; i < count_ccg_query_fields; i++)
  {
    table->column(i, CCG_QUERY_FIELDS[i]);
  }
  Table::Row row(count_ccg_query_fields);
  add_query_header_("all_ccg", reference_.c_str(), row);
  row.add_field(date_.get_gm_time().format("%Y-%m-%d-%H:%M:%S"));
  row.add_field(*ulong_options[OPT_FIRST].value);
  row.add_field(*ulong_options[OPT_LIMIT].value);
  row.add_field(check_options[OPT_ONLY_LIST].value.enabled() ? "yes" : "no");
  table->add_row(row);
  table->dump(std::cout);
}

void Application::print_ccg_answer_header_(
  int source_id,
  unsigned long start_id)
  noexcept
{
  std::unique_ptr<Table> table(new Table(2));
  Table::Row row(2);
  table->column(0, Table::Column("source"));
  table->column(1, Table::Column("start_id", Table::Column::NUMBER));
  row.add_field(source_id);
  row.add_field(start_id);
  table->add_row(row);
  table->dump(std::cout);
}

void Application::print_ccg_deleted_(
  const AdServer::ChannelSvcs::ChannelCurrent::TriggerVersionSeq& deleted)
  noexcept
{
  if(deleted.length())
  {
    std::unique_ptr<Table> table(new Table(2));
    Table::Row row(2);
    table->column(0, Table::Column("erased_id", Table::Column::NUMBER));
    table->column(1, Table::Column("erased_stamp"));
    Generics::Time stamp;
    size_t i = 0;
    do
    {
      stamp = CorbaAlgs::unpack_time(deleted[i].stamp); 
      row.add_field(deleted[i].id);
      row.add_field(stamp.get_gm_time().format("%Y-%m-%d-%H:%M:%S"));
      table->add_row(row);
      row.clear();
    }
    while(++i < deleted.length());
    table->dump(std::cout);
  }
}

template<class T>
void print_ccg_keywords(
  const T& ccg_keywords,
  const Table::Filters& filters)
{
  const size_t count_fields =
    sizeof(CCG_KEYWORDS_FIELDS)/sizeof(CCG_KEYWORDS_FIELDS[0]);
  std::unique_ptr<Table> table(new Table(count_fields));
  for(size_t i = 0; i < count_fields; i++)
  {
    table->column(i, CCG_KEYWORDS_FIELDS[i]);
  }
  for(size_t j = 0; j < ccg_keywords.length(); j++)
  {
    Table::Row row(count_fields);
    row.add_field(ccg_keywords[j].ccg_keyword_id);
    row.add_field(ccg_keywords[j].ccg_id);
    row.add_field(ccg_keywords[j].channel_id);
    row.add_field(
      CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
        ccg_keywords[j].max_cpc).str());
    row.add_field(
      CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::CTRDecimal>(
        ccg_keywords[j].ctr).str());
    row.add_field(ccg_keywords[j].click_url);
    row.add_field(ccg_keywords[j].original_keyword);
    table->add_row(row, filters);
  }
  table->dump(std::cout);
}

int Application::pos_ccg_()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::SystemException)*/
{
  init_update_interface_();
  AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_var result;
  AdServer::ChannelSvcs::ChannelCurrent::CCGQuery in;
  in.master_stamp = CorbaAlgs::pack_time(date_);
  std::cout << "in.master_stamp.length = " << in.master_stamp.length() << std::endl;
  in.start = *ulong_options[OPT_FIRST].value;
  in.limit = *ulong_options[OPT_LIMIT].value;

  {
    channels_id_.clear();
    for(int i = 0; i < 1/*434686*/; ++i)
    {
      channels_id_.emplace_back(i);
    }
  }
  
  in.channel_ids.length(channels_id_.size());
  std::copy(
    channels_id_.begin(),
    channels_id_.end(),
    in.channel_ids.get_buffer());
  in.use_only_list = check_options[OPT_ONLY_LIST].value.enabled();
  StatMarker stat_marker(
    *string_options[OPT_STAT].value,
    "All_ccg",
    *ulong_options[OPT_TIMES].value);
  try
  {
    if (use_session_)
    {
      /*
      for(int i = 0; i < 2; ++i)
      {
        AdServer::ChannelSvcs::ChannelCurrent::CheckQuery check_in;
        check_in.colo_id = 1;
        check_in.master_stamp = CorbaAlgs::pack_time(Generics::Time::get_time_of_day());
        check_in.use_only_list = false;

        AdServer::ChannelSvcs::ChannelCurrent::CheckData_var check_result;

        typedef
          decltype(&AdServer::ChannelSvcs::ChannelLoadSession::check)
          FuncType;
        FuncType func_ptr =
          &AdServer::ChannelSvcs::ChannelLoadSession::check;
        stat_marker.calc_stat(&*load_session_, func_ptr, check_in, check_result);
      }
      */

      for(int i = 0; i < 2; ++i)
      {
        std::cout << "to update_all_ccg step #" << i << std::endl;

        typedef
          decltype(&AdServer::ChannelSvcs::ChannelLoadSession::update_all_ccg)
          FuncType;
        FuncType func_ptr =
          &AdServer::ChannelSvcs::ChannelLoadSession::update_all_ccg;
        stat_marker.calc_stat(&*load_session_, func_ptr, in, result);

        std::cout << "from update_all_ccg step #" << i << std::endl;
      }
    }
    else
    {
      typedef
        decltype(&AdServer::ChannelSvcs::ChannelUpdateBase_v33::update_all_ccg)
        FuncType;
      FuncType func_ptr =
        &AdServer::ChannelSvcs::ChannelUpdateBase_v33::update_all_ccg;
      stat_marker.calc_stat(&*channel_update_, func_ptr, in, result);
    }
  }
  catch(const AdServer::ChannelSvcs::ImplementationException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": ImplementationException: " << e.description;
    throw Exception(ostr);
  }
  catch(const AdServer::ChannelSvcs::NotConfigured& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": NotConfigured: " << e.description;
    throw Exception(ostr);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": CORBA::SystemException: " << e;
    throw Exception(ostr);
  }
  print_ccg_query_params_();
  print_ccg_answer_header_(result->source_id, result->start_id);
  print_ccg_deleted_(result->deleted);
  print_ccg_keywords(result->keywords, filters_);
  return 0;
}

int Application::ccg_traits_()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::SystemException)*/
{
  init_server_interface_();
  {
    std::unique_ptr<Table> table(new Table(count_ccg_traits_query_fields));
    for(size_t i = 0; i < count_ccg_traits_query_fields; i++)
    {
      table->column(i, CCG_TRAITS_QUERY_FIELDS[i]);
    }
    Table::Row row(count_ccg_traits_query_fields);
    add_query_header_("ccg_traits", reference_.c_str(), row);
    row.add_field(concat_sequence(channels_id_.begin(), channels_id_.end()));
    table->add_row(row);
    table->dump(std::cout);
  }
  AdServer::ChannelSvcs::ChannelIdSeq in;
  AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq_var result_1;
  AdServer::ChannelSvcs::ChannelServer::TraitsResult_var result_2;
  in.length(channels_id_.size());
  std::copy(channels_id_.begin(), channels_id_.end(), in.get_buffer());
  StatMarker stat_marker(
    *string_options[OPT_STAT].value,
    "Ccg_traits",
    *ulong_options[OPT_TIMES].value);
  try
  {
    if (use_session_)
    {
      typedef
        decltype(&AdServer::ChannelSvcs::ChannelServerSession::get_ccg_traits)
        FuncType;
      FuncType func_ptr =
        &AdServer::ChannelSvcs::ChannelServerSession::get_ccg_traits;
      stat_marker.calc_stat_r(&*channel_session_, func_ptr, result_1, in);
    }
    else
    {
      typedef
        decltype(&AdServer::ChannelSvcs::ChannelServer::get_ccg_traits)
        FuncType;
      FuncType func_ptr =
        &AdServer::ChannelSvcs::ChannelServer::get_ccg_traits;
      stat_marker.calc_stat(&*channel_server_, func_ptr, in, result_2);
      result_1 = new AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq(
        result_2->ccg_keywords);
      std::unique_ptr<Table> table(new Table(1));
      Table::Row row(1);
      table->column(0, Table::Column("negative_ccg"));
      row.add_field(
        concat_sequence(
          result_2->neg_ccg.get_buffer(),
          result_2->neg_ccg.get_buffer() + result_2->neg_ccg.length()));
      table->add_row(row);
      table->dump(std::cout);
    }
  }
  catch(const AdServer::ChannelSvcs::ImplementationException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": ImplementationException: " << e.description;
    throw Exception(ostr);
  }
  catch(const AdServer::ChannelSvcs::NotConfigured& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": NotConfigured: " << e.description;
    throw Exception(ostr);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": CORBA::SystemException: " << e;
    throw Exception(ostr);
  }
  print_ccg_keywords(*result_1, filters_);
  return 0;
}

int Application::lexemes_()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::SystemException)*/
{
  const char* FN = "Application::lexemes_";
  AdServer::ChannelSvcs::DictionaryProvider_var service;
  try
  {
    if (string_options[OPT_REFERENCE].value.installed())
    {
      reference_ = *string_options[OPT_REFERENCE].value;
    }
    else
    {
      reference_ = "corbaloc:iiop:";
      reference_ += *string_options[OPT_HOST_AND_PORT].value;
      reference_ += "/DictionaryProvider";
    }
    obj_ref_ = adapter_->resolve_object(reference_.c_str());

    if (CORBA::is_nil(obj_ref_))
    {
      Stream::Error ostr;
      ostr << "reference is null";
      throw InvalidArgument(ostr);
    }

    service = AdServer::ChannelSvcs::DictionaryProvider::_narrow(obj_ref_.in());

    if(CORBA::is_nil(service))
    {
      Stream::Error err;
      err << FN << "ChannelSvcs::DictionaryProvider::_narrow failed ";
      throw InvalidArgument(err);
    }
  }
  catch(const CORBACommons::CorbaClientAdapter::Exception& e)
  {
    Stream::Error ostr;
    ostr << "Failed to resolve dictionary provider reference." 
      " CorbaClientAdapter::Exception: " << e.what();
    throw InvalidArgument(ostr);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << FN << "failed to resolve dictionary provider reference." 
      " CORBA::SystemException: " << e;
    throw InvalidArgument(ostr);
  }
  std::unique_ptr<Table> table(new Table(count_lexemes_query_fields));
  for(size_t i = 0; i < count_lexemes_query_fields; i++)
  {
    table->column(i, LEXEMES_QUERY_FIELDS[i]);
  }
  std::string lang;
  if(string_options[OPT_LANG].value.installed())
  {
    lang = *string_options[OPT_LANG].value;
  }
  Table::Row row(count_lexemes_query_fields);
  add_query_header_("lexemes", reference_.c_str(), row);
  row.add_field(lang.c_str());
  row.add_field(concat_sequence(lexemes_data_.begin(), lexemes_data_.end()));
  table->add_row(row);
  table->dump(std::cout);
  ::CORBACommons::StringSeq words;
  words.length(lexemes_data_.size());
  for(size_t i = 0; i < lexemes_data_.size(); i++)
  {
    words[i] << lexemes_data_[i];
  }
  ::AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq_var res =
    service->get_lexemes(lang.c_str(), words); 
  table.reset(new Table(count_lexemes_fields));
  for(size_t i = 0; i < count_lexemes_fields; i++)
  {
    table->column(i, LEXEMES_FIELDS[i]);
  }
  for(size_t i = 0; i < res->length(); i++)
  {
    Table::Row row(count_lexemes_fields);
    row.add_field(words[i]);
    row.add_field(concat_sequence(
        (*res)[i].forms.get_buffer(),
        (*res)[i].forms.get_buffer() + (*res)[i].forms.length()));
    table->add_row(row);
  }
  table->dump(std::cout);
  return 0;
}


template<class ITER>
std::string Application::concat_sequence(ITER begin, ITER end)
  noexcept
{
  std::ostringstream ostr;
  ITER i = begin;
  while(i < end)
  {
    if(i != begin)
    {
      ostr << ", ";
    }
    ostr << *i;
    i++;
  }
  return ostr.str();
}

void add_row_to_match_result(
  const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtomSeq& channels,
  char type,
  const Table::Filters& filters,
  Table* table)
{
  for(size_t i = 0; i < channels.length(); i++)
  {
    const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom& channel =
      channels[i];
    Table::Row row(3);
    row.add_field(channel.id);
    row.add_field(channel.trigger_channel_id);
    row.add_field(type);
    table->add_row(row, filters);
  }
}

void add_row_to_match_result(
  const AdServer::ChannelSvcs::ChannelIdSeq& channels,
  char type,
  const Table::Filters& filters,
  Table* table)
{
  for(size_t i = 0; i < channels.length(); i++)
  {
    const CORBA::ULong& channel = channels[i];
    Table::Row row(3);
    row.add_field(channel);
    row.add_field(channel);
    row.add_field(type);
    table->add_row(row, filters);
  }
}

template<class T>
void print_match_result(T& result, const Table::Filters& filters, bool no_print_content)
{
  std::unique_ptr<Table> table(new Table(2));
  table->column(0, Table::Column("NO_ADV"));
  table->column(1, Table::Column("NO_TRACK"));
  Table::Row row(2);
  row.add_field(result->no_adv?"yes":"no");
  row.add_field(result->no_track?"yes":"no");
  table->add_row(row);
  table->dump(std::cout);

  table.reset(new Table(3));
  table->column(0, Table::Column("channel_id", Table::Column::NUMBER));
  table->column(1, Table::Column("trigger_channel_id", Table::Column::NUMBER));
  table->column(2, Table::Column("type"));
  add_row_to_match_result(
    result->matched_channels.page_channels, 'P', filters, table.get());
  add_row_to_match_result(
    result->matched_channels.search_channels, 'S', filters, table.get());
  add_row_to_match_result(
    result->matched_channels.url_channels, 'U', filters, table.get());
  add_row_to_match_result(
    result->matched_channels.url_keyword_channels, 'R', filters, table.get());
  add_row_to_match_result(
    result->matched_channels.uid_channels, 'A', filters, table.get());
  table->dump(std::cout);
  if(!no_print_content)
  {
    table.reset(new Table(2));
    table->column(0, Table::Column("content_id", Table::Column::NUMBER));
    table->column(1, Table::Column("content_weight"));
    for(size_t j = 0; j < result->content_channels.length(); j++)
    {
      Table::Row row(2);
      row.add_field(result->content_channels[j].id);
      row.add_field(result->content_channels[j].weight);
      table->add_row(row);
    }
    table->dump(std::cout);
  }
}

void Application::print_unmatched(
  const char* name,
  const CORBACommons::StringSeq& unmatched)
  /*throw(eh::Exception)*/
{
  std::unique_ptr<Table> table(new Table(1));
  table->column(0, name);
  for(size_t j = 0; j < unmatched.length(); j++)
  {
    Table::Row row(1);
    if(unmatched[j][0] != '\0')
    {
      row.add_field(unmatched[j]);
      table->add_row(row);
    }
  }
  table->dump(std::cout);
}

template<class T>
void print_match_appendix(
  const std::string& match_time,
  const T& result)
{
  std::unique_ptr<Table> table(new Table(3));
  Table::Row row(3);
  table->column(0, Table::Column("NO_ADV", Table::Column::NUMBER));
  table->column(1, Table::Column("NO_TRACK", Table::Column::NUMBER));
  table->column(2, Table::Column("match_time"));
  row.add_field((result->no_adv ? 1: 0));
  row.add_field((result->no_track ? 1: 0));
  row.add_field(match_time);
  table->add_row(row);
  table->dump(std::cout);
}

template<class T1, class T2>
void Application::make_match_query(
  T1* iface_ptr,
  T2& res)
  /*throw(Exception)*/
{
  AdServer::ChannelSvcs::ChannelServerBase::MatchQuery in; 
  try
  {
    Generics::Uuid uid;
    in.request_id << String::SubString("ChannelAdmin");
    in.first_url << *string_options[OPT_URL].value;
    in.pwords << *string_options[OPT_PWORDS].value;
    in.swords << *string_options[OPT_SWORDS].value;
    if (!string_options[OPT_UID].value.installed())
    {
      in.uid = CorbaAlgs::pack_user_id(Generics::Uuid());
    }
    else
    {
      in.uid = CorbaAlgs::pack_user_id(
        Generics::Uuid(*string_options[OPT_UID].value, true));
    }
    in.non_strict_word_match =
      check_options[OPT_NS_WORD_H].value.enabled() ||
      check_options[OPT_NS_WORD_S].value.enabled();
    in.non_strict_url_match = check_options[OPT_NS_WORD_U].value.enabled();
    in.return_negative = check_options[OPT_NEGATIVE].value.enabled();
    in.simplify_page = !check_options[OPT_NO_SIMPLIFY_PAGE].value.enabled();
    in.fill_content = !check_options[OPT_NO_PRINT_CONTENT].value.enabled();
    if (string_options[OPT_STATUS].value->empty())
    {
      throw Exception("statuses didn't set");
    }
    in.statuses[0] = (*string_options[OPT_STATUS].value)[0];
    if (string_options[OPT_STATUS].value->size() > 1)
    {
      in.statuses[1] = (*string_options[OPT_STATUS].value)[1];
    }
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": Exception: " << e.what();
    throw Exception(ostr);
  }
  StatMarker stat_marker(
    *string_options[OPT_STAT].value,
    "Match",
    *ulong_options[OPT_TIMES].value);
  typedef decltype(&T1::match) FuncType;
  FuncType func_ptr = &T1::match;
  try
  {
    stat_marker.calc_stat(iface_ptr, func_ptr, in, res);
  }
  catch(const AdServer::ChannelSvcs::ImplementationException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": ImplementationException: " << e.description;
    throw Exception(ostr);
  }
  catch(const AdServer::ChannelSvcs::NotConfigured& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": NotConfigured: " << e.description;
    throw Exception(ostr);
  }
}

int Application::match_()
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::SystemException)*/
{
  init_server_interface_();
  {
    if(!check_options[OPT_WILD_INPUT].value.enabled())
    {
      try
      {
        HTTP::BrowserAddress addr(*string_options[OPT_URL].value);
        std::string res;
        addr.get_view(HTTP::HTTPAddress::VW_FULL, res);
        Generics::AppUtils::StringOption opt(res);
        string_options[OPT_URL].value = opt;
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << __func__ << ": " << e.what();
        throw InvalidArgument(err);
      }
    }
    std::unique_ptr<Table> table(new Table(count_match_query_fields));
    for(size_t i = 0; i < count_match_query_fields; i++)
    {
      table->column(i, MATCH_QUERY_FIELDS[i]);
    }
    Table::Row row(count_match_query_fields);
    add_query_header_("match", reference_.c_str(), row);
    row.add_field(check_options[OPT_NS_WORD_H].value.enabled() ||
      check_options[OPT_NS_WORD_S].value.enabled() ? "yes" : "no");
    row.add_field(check_options[OPT_NS_WORD_U].value.enabled() ? "yes" : "no");
    row.add_field(*string_options[OPT_PWORDS].value);
    row.add_field(*string_options[OPT_SWORDS].value);
    row.add_field(*string_options[OPT_URL].value);
    row.add_field(*string_options[OPT_UID].value);
    table->add_row(row);
    table->dump(std::cout);
  }
  std::ostringstream match_time;
  std::string err_descr;
  try
  {
    if (use_session_)
    {
      AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var result_1; 
      AdServer::ChannelSvcs::ChannelIdSeq empty;
      make_match_query<
        AdServer::ChannelSvcs::ChannelServerSession,
        AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var>(
          channel_session_, result_1);
        print_match_result(
          result_1,
          filters_, check_options[OPT_NO_PRINT_CONTENT].value.enabled());
        for(size_t i = 0; i < result_1->match_time.length(); i++)
        {
          match_time << CorbaAlgs::unpack_time(result_1->match_time[i]);
          if(i + 1 == result_1->match_time.length())
          {
            match_time << ".";
          }
          else
          {
            match_time << ", ";
          }
        }
        print_match_appendix(match_time.str(), result_1);
    }
    else
    {
      AdServer::ChannelSvcs::ChannelServer::MatchResult_var result_2; 
      make_match_query<
        AdServer::ChannelSvcs::ChannelServer,
        AdServer::ChannelSvcs::ChannelServer::MatchResult_var>(
          channel_server_, result_2);
        print_match_result(
          result_2,
          filters_, check_options[OPT_NO_PRINT_CONTENT].value.enabled());
        match_time << CorbaAlgs::unpack_time(result_2->match_time);
        print_match_appendix(match_time.str(), result_2);
    }
  }
  catch(const Exception& e)
  {
    err_descr = e.what();
  }
  if(err_descr.size())
  {
    throw Exception(err_descr);
  }
  return 0;
}

Application::StatMarker::StatMarker(
  const std::string& option, const char* name, unsigned long times) noexcept
  : option_(STAT_NO),
    times_(times)
{
  stat_runner_ = new Generics::Statistics::NullDumpRunner;
  collection_ = new Generics::Statistics::Collection(stat_runner_.in());
  policy_ = new Generics::Statistics::NullDumpPolicy;
  if(option == "norm" || option == "all")
  {
    option_ = STAT_NORM;
    main_name_ = std::string(name) + "_statistic";
    collection_->add(
      main_name_.c_str(),
      new Generics::Statistics::TimedStatSink(),
      policy_.in());//TimedQueryStat
  }
  if(option == "nomax" || option == "all")
  {
    option_ = STAT_NOMAX;
    no_max_name_ = std::string(name) + "_statistic_without_max";
    collection_->add(
      no_max_name_.c_str(),
      new Generics::Statistics::TimedStatSink(),
      policy_.in());//TimedQueryStat
  }
  if(option == "nofirst" || option == "all")
  {
    option_ = STAT_NOFIRST;
    no_first_name_ = std::string(name) + "_statistic_without_first";
    collection_->add(
      no_first_name_.c_str(),
      new Generics::Statistics::TimedStatSink(),
      policy_.in());//TimedQueryStat
  }
  if(option == "all")
  {
    option_ = STAT_ALL;
  }
  if(option_ == STAT_NO)
  {
    stat_runner_.reset();
    collection_.reset();
    policy_.reset();
  }
}


Application::StatMarker::~StatMarker() noexcept
{
  if(collection_)
  {
    collection_->dump(std::cout);
  }
}

void Application::StatMarker::calc_value_(
  Generics::Time&& elapsed_time, size_t iteration)
  noexcept
{
  if(collection_)
  {
    if(option_ == STAT_ALL || option_ == STAT_NORM)
    {
      Generics::Statistics::StatSink_var stat(
        collection_->get(main_name_.c_str()));
      stat->consider(Generics::Statistics::TimedSubject(elapsed_time));
    }
    if(iteration != 0 && (option_ == STAT_ALL || option_ == STAT_NOFIRST))
    {
      Generics::Statistics::StatSink_var stat(
        collection_->get(no_first_name_.c_str()));
      stat->consider(Generics::Statistics::TimedSubject(elapsed_time));
    }
    if(option_ == STAT_ALL || option_ == STAT_NOMAX)
    {
      if(iteration == 0)
      {
        max_time_ = elapsed_time;
        return;
      }
      if(elapsed_time > max_time_)
      {
        std::swap(max_time_, elapsed_time);
      }
      Generics::Statistics::StatSink_var stat(
        collection_->get(no_max_name_.c_str()));
      stat->consider(Generics::Statistics::TimedSubject(elapsed_time));
    }
  }
}


