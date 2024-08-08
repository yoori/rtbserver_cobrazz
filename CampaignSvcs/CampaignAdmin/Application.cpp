#include <iostream>
#include <sstream>
#include <string>

#include <String/StringManip.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/Xslt/XslTransformer.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Generics/AppUtils.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelCorbaAdapter.hpp>
#include <Frontends/FrontendCommons/OptOutManip.hpp>

#include "Application.hpp"

namespace
{
  const char TIME_FORMAT[] = "%d-%m-%Y:%H-%M-%S.%q";
}

namespace AdminTools
{
  void
  create_prefix(std::string& prefix, const Table::Column* source,
    unsigned long table_length)
  {
    //This parameter increases length of created prefix;
    std::string::size_type ADDITION = 3;

    std::string::size_type max_len = 0;
    for (unsigned long ind = 0; ind < table_length; ind++)
    {
      std::string::size_type new_len = source[ind].name.length();
      if (max_len < new_len)
      {
        max_len = new_len;
      }
    }

    max_len += ADDITION;

    prefix.assign(max_len, ' ');
  }
}

namespace
{
  const char USAGE[] =
    "Usage:\nCampaignAdmin <command> <options> <filters>\n\n"
    " Common options ::= \n\t-e[--expand=]\t\t"
    "option will cause most detailed information to be displayed.\n"
    "\t-r[--reference=]'\t"
    "CORBA reference to the CampaignManager or CampaignServer\n"
    "\t-P[--host_and_port=]'\t"
    "port and host for CORBA reference to the CampaignManager or CampaignServer,"
    " this option uses when CORBA reference doesn't set\n"
    "\t-o[--show=]'\t\tsame as filters\n"
    "\t-O[--sort=]'\t\t"
    "sort option ::= sort<rel op><field name> // if rel op is '<', "
    "then sorting in descending order, otherwise in ascending\n"
    "  <filters> ::= <field name><rel op><value>\n"
    "  rel op ::= ~ | !~ | = | != | < | > | <= | >=  // '~' relational operator means \"contained\"\n"
    "Synopsis 1: (viewing entities)\n"
    "  command ::= globals, creative_categories, sites, colocations, countries, "
    "creative_templates, campaign, tags,\ncurrency, creative, freq-cap, "
    "  creative-option, adv_action, category_channel, geo_channel, geo_coord_channel\n"
    "web_operations, keyword, account, fraud_condition\n"
    " options ::= \n"
    "\t-x[--expand_options]  "
    "option will cause multistring options to be displayed without cutting.\n"
    "  campaign/creative information; external campaigns ecpm depends on words passed\n\n"
    "Samples:\n\n"
    "CampaignAdmin campaign id=2345  -r corbaloc::localhost:10107/CampaignManager\n"
    "  Show campaign with id equal 2345\n\n"
    "CampaignAdmin campaign advertiser_id=856  -r corbaloc::localhost:10107/CampaignManager\n"
    "  Show campaigns from advertiser with id equal 856\n\n"
    "CampaignAdmin campaign creatives~60976 status=A --sort\\<budget --reference=corbaloc::localhost:10107/CampaignManager\n"
    "  Show active campaigns containing creavive with id equal 60976, sort decreasing budget\n\n"
    "CampaignAdmin campaign countries~US ron_flag=TARGETED -e -r corbaloc::localhost:10107/CampaignManager\n"
    "  Show targeted campaigns containing \"US\" among its contries, show expanded info (dereference freq caps ids)\n\n"
    "CampaignAdmin creative id=1342 -e -r corbaloc::localhost:10107/CampaignManager\n"
    "  Show creative with id equal 1342, show expanded info (dereference freq caps ids and campaign id)\n\n"
    "NOTE: Working in shell do not forget to prepend special characters "
    "'!', '>' and '<' with '\\'.\n      So write ecpm\\>1000 instead of just ecpm>1000\n\n"
    "Synopsis 2 (checking creative selection logic):\n"
    "CampaignAdmin select <options>\n\n"
    " options ::= \n\t"
    "-f[--freq_caps=]\t\t <freq cap cookie> user freq cap cookie\n\t"
    "-F[--uc_freq_caps=]\t <uc freq cap cookie> user uc freq cap cookie\n\t"
    "-J[--format=]\t\t <format> required format (html by default)\n\t"
    "-c[--country=]\t\t <country code> country code\n\t"
    "-n[--colo_id=]\t\t <colocation id>] id of colocation\n\t"
    "-t[--tid=]\t\t<tag ids> sequence of ids separated by ','>\n\t"
    "-u[--uid=]\t\t<user id> id of user\n\t"
    "-R[--referer=]\t\t<referer> referer]\n\t"
    "-C[--client_version=]\t\t <client version> client version\n\t"
    "-p[--peer_ip=]\t\t <peer ip>] IP adress of peed\n\t"
    "-V[--os_version=]\t\t<os version> os version\n\t"
    "-A[--app=]\t\t<application name>]\n\t"
    "-N[--random=]\t\t<random> value for %%RANDOM%% token instaniating\n\t"
    "-E[--optout=]\t\t<1|0> to consider optout ad blocking>]\n\t"
    "-X[--xinfo=]\t\t<extended info>]\n\t"
    "-B[--browser_version=]\t\t<browser version>]\n\t"
    "-I[--channels=]<triggered_channels> channel id separated by coma\n\n"
    "Sample:\n\n"
    "CampaignAdmin select --uid=\"PPPPPPPPPPPPPPPPPPPPPA..\" -t 1 "
      "-r corbaloc::localhost:10107/CampaignManager\n\n"
    "Synopsis 3 (checking just creative instantiation):\n"
    "CampaignAdmin preview <options>\n"
    "options ::= \n\t"
    "-i[--id=]<campaign creative id>\n\t"
    "-J[--format=]<required format>\n\t"
    "-R[--referer=]<url>]\n\t"
    "-p[peer_ip=]<ip>\n\n"
    "Sample:\n\n"
    "CampaignAdmin preview --id=6397 --reference=corbaloc::localhost:10107/CampaignManager\n\n"
    "Synopsis 4 (tracing campaign weighting process):\n"
    "CampaignAdmin trace-weight <options>\n\t"
    "-i[--id=]<campaign_id for tracing>\n\t"
    "-J[--format=]<required format>\n\t"
    "-c[--country=]<country code>\n\t"
    "-t[--tid=]<tag id>\n\t"
    "-R[--referer]=<url>\n\t"
    "-I[--channels=]<triggered_channels>, list channels id separated by coma'\n\t"
    "-n[--colo_id=]<colo_id>]\n\t"
    "-x[--xsl=]<xsl schema for output presenting>\n\t"
    "-E[--optout=]<0|1>\n\t"
    "-T[--testrequest]=<0|1|2>\n\n"
    "  xsl schema for simple plain text viewing is Plain.xsl\n\n"
    "Sample:\n\n"
    "CampaignAdmin trace_weight -i 98311 --format=html --country=RU --tid=1 "
    "-R google.ru --channels=1,2 -r corbaloc::localhost:10107/CampaignManager\n\n"
    "Synopsis 5 (get expression channels):\n"
    "CampaignAdmin expression_channel -r corbaloc::localhost:10106/CampaignServer\n\n"
    "Synopsis 6 (get simple channels):\n"
    "CampaignAdmin simple_channel -r corbaloc::localhost:10106/CampaignServer\n\n"
    "Synopsis 7 (tracing campaign index tree):\n"
    "CampaignAdmin trace_index -r corbaloc::localhost:10107/CampaignManager\n\n"
    "Synopsis 9 (get behav parmam info):\n"
    "CampaignAdmin behav_param -r corbaloc::localhost:10106/CampaignServer\n\n"
    "Synopsis 10 (get behav param keys):\n"
    "CampaignAdmin behav_keys -r corbaloc::localhost:10106/CampaignServer\n\n"
    "Synopsis 11 (get search engines):\n"
    "CampaignAdmin search_engines <options>\n\t"
    "options ::= \n\t"
    "-D[--deleted] show deleted search engines\n\t"
    "-U[--update] use update interface for query\n\n"
    "CampaignAdmin search_engines -r corbaloc::localhost:10106/CampaignServer -U\n\n"
    "Synopsis 12 (get colocation flags):\n"
    "CampaignAdmin colocation-flags -r corbaloc::localhost:10107/CampaignManager\n\n"
    "Synopsis 13 (get detectors):\n"
    "CampaignAdmin detectors <options>\n\t"
    "options ::= \n\t"
    "-w[--web] show web browser detectors\n\t"
    "-l[--platform] show platform detectors\n\n"
    "CampaignAdmin detectors -w -r corbaloc::localhost:10107/CampaignManager\n\n"
    "Synopsis 14 (decode trial_opt_in cookie):\n"
    "CampaignAdmin decode --trialoptin=<value>\n\n"
    "Synopsis 15 (get statistic):\n"
    "CampaignAdmin [stat_ccg|stat_account|stat_campaign]\n\n"
    "CampaignAdmin stat_ccg -r corbaloc::localhost:10107/CampaignManager\n\n"
    "Synopsis 16 (get simple channels for channel server):\n"
    "CampaignAdmin simple_channel_cs -r corbaloc::localhost:10106/CampaignServer\n\n"
    "Synopsis 17 (get bill stats accounts):\n"
    "CampaignAdmin bill_accounts -r corbaloc::localhost:10106/CampaignServer\n\n"
    "Synopsis 18 (get bill stats campaigns):\n"
    "CampaignAdmin bill_campaigns -r corbaloc::localhost:10106/CampaignServer\n\n"
    "Synopsis 18 (get bill stats ccgs):\n"
    "CampaignAdmin bill_ccgs -r corbaloc::localhost:10106/CampaignServer\n\n";

  const char *COMMAND[Application::command_max] =
  {
    "help",
    "globals",
    "creative_categories",
    "sites",
    "colocations",
    "countries",
    "creative_templates",
    "campaign",
    "tags",
    "currency",
    "creative",
    "freq_cap",
    "creative_option",
    "adv_action",
    "category_channel",
    "geo_channel",
    "geo_coord_channel",
    "web_operations",
    "keyword",
    "account",
    "fraud_condition",
    "select",
    "preview",
    "expression_channel",
    "simple_channel",
    "simple_channel_cs",
    "trace_index",
    "trace_weight",
    "behav_param",
    "behav_keys",
    "search_engines",
    "colocation_flags",
    "detectors",
    "stat",
    "stat_ccg",
    "stat_account",
    "stat_campaign",
    "update_stat",
    "decode",
    "app_format",
    "size",
    "bill_accounts",
    "bill_campaigns",
    "bill_ccgs",
    "contract"
  };

  template <class T>
  struct Option
  {
    const unsigned long id;
    const char* const equal_name;
    const char* const short_name;
    T value;

    Option(unsigned long id, const char* equal_name,
      const char* short_name) /*throw(eh::Exception)*/;
    template <typename Init>
    Option(unsigned long id, const char* equal_name,
      const char* short_name, Init init) /*throw(eh::Exception)*/;
  };

  template <class T>
  Option<T>::Option(unsigned long id, const char* equal_name,
    const char* short_name) /*throw(eh::Exception)*/
    : id(id), equal_name(equal_name), short_name(short_name)
  {
  }

  template <class T>
  template <typename Init>
  Option<T>::Option(unsigned long id, const char* equal_name,
    const char* short_name, Init init) /*throw(eh::Exception)*/
    : id(id), equal_name(equal_name), short_name(short_name), value(init)
  {
  }

  enum CheckOptionId
  {
    OPT_SERVER = 0,
    OPT_EXPAND,
    OPT_EXPAND_OPT,
    OPT_PLATFORM,
    OPT_WEB,
    OPT_DELETED,
    OPT_UPDATE,
    OPT_CHECK_MAX
  };

  typedef Option<Generics::AppUtils::CheckOption> OptionCheck;
  static OptionCheck check_options[] =
  {
    OptionCheck(OPT_SERVER, "server", "S"),
    OptionCheck(OPT_EXPAND, "expand", "e"),
    OptionCheck(OPT_EXPAND_OPT, "expand_options", "x"),
    OptionCheck(OPT_PLATFORM, "platform", "l"),
    OptionCheck(OPT_WEB, "web", "w"),
    OptionCheck(OPT_DELETED, "deleted", "D"),
    OptionCheck(OPT_UPDATE, "update", "U")
  };

  enum StringOptionId
  {
    OPT_FREQ_CAPS,
    OPT_UC_FREQ_CAPS,
    OPT_FORMAT,
    OPT_COUNTRY,
    OPT_TID,
    OPT_UID,
    OPT_REFERER,
    OPT_REFERENCE,
    OPT_HOST_AND_PORT,
    OPT_CLIENT_VERSION,
    OPT_IP,
    OPT_OS,
    OPT_APP,
    OPT_RANDOM,
    OPT_XINFO,
    OPT_BROWSER,
    OPT_CHANNELS,
    OPT_FILTER,
    OPT_SORTER,
    OPT_TRIAL_OPT_IN,
    OPT_XSL,
    OPT_STRING_MAX
  };

  typedef Option<Generics::AppUtils::StringOption> OptionString;
  static OptionString string_options[] =
  {
    OptionString(OPT_FREQ_CAPS, "freq_caps", "f"),
    OptionString(OPT_UC_FREQ_CAPS, "uc_freq_caps", "F"),
    OptionString(OPT_FORMAT, "format", "J", "html"),
    OptionString(OPT_COUNTRY, "country", "c"),
    OptionString(OPT_TID, "tid", "t"),
    OptionString(OPT_UID, "uid", "u"),
    OptionString(OPT_REFERER, "referer", "R"),
    OptionString(OPT_REFERENCE, "reference", "r"),
    OptionString(OPT_HOST_AND_PORT, "host_and_port", "P"),
    OptionString(OPT_CLIENT_VERSION, "client_version", "C"),
    OptionString(OPT_IP, "peer_ip", "p"),
    OptionString(OPT_OS, "os_version", "V"),
    OptionString(OPT_APP, "app", "A"),
    OptionString(OPT_RANDOM, "random", "N"),
    OptionString(OPT_XINFO, "xinfo", "X"),
    OptionString(OPT_BROWSER, "browser_version", "B"),
    OptionString(OPT_CHANNELS, "channels", "I"),
    OptionString(OPT_FILTER, "show", "o"),
    OptionString(OPT_SORTER, "sort", "O"),
    OptionString(OPT_TRIAL_OPT_IN, "trialoptin", "a"),
    OptionString(OPT_XSL, "xsl", "x")
  };

  enum ULongOptionId
  {
    OPT_COLO = 0,
    OPT_ID,
    OPT_OPTOUT,
    OPT_TEST_REQ,
    OPT_ULONG_MAX
  };

  typedef Option<Generics::AppUtils::Option<unsigned long> > OptionULong;

  static OptionULong ulong_options[] =
  {
    OptionULong(OPT_COLO, "colo_id", "n", 1),
    OptionULong(OPT_ID, "id", "i", 0),
    OptionULong(OPT_OPTOUT, "optout", "E", 0),
    OptionULong(OPT_TEST_REQ, "testrequest", "T", 0)
  };

  const Table::Column APP_FORMAT_COLUMNS[] =
  {
    Table::Column("app_format", Table::Column::TEXT),
    Table::Column("mime_format", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column ACCOUNT_TABLE_COLUMNS[] =
  {
    Table::Column("account_id", Table::Column::NUMBER),
    Table::Column("agency_account_id", Table::Column::NUMBER),
    Table::Column("internal_account_id", Table::Column::NUMBER),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("eval_status", Table::Column::TEXT),
    Table::Column("legal_name", Table::Column::TEXT),
    Table::Column("flags", Table::Column::NUMBER),
    Table::Column("at_flags", Table::Column::TEXT),
    Table::Column("text_adserving", Table::Column::TEXT),
    Table::Column("currency_id", Table::Column::NUMBER),
    Table::Column("country", Table::Column::TEXT),
    Table::Column("commision", Table::Column::TEXT),
    Table::Column("budget", Table::Column::TEXT),
    Table::Column("paid_amount", Table::Column::TEXT),
    Table::Column("time_offset", Table::Column::TEXT),
    Table::Column("walled_garden_accounts", Table::Column::TEXT),
    Table::Column("auction_rate", Table::Column::TEXT),
    Table::Column("use_pub_pixels", Table::Column::NUMBER),
    Table::Column("pub_pixel_optin", Table::Column::TEXT),
    Table::Column("pub_pixel_optout", Table::Column::TEXT),
    Table::Column("self_service_commission", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column CREATIVE_OPTION_TABLE_COLUMNS[] =
  {
    Table::Column("option_id", Table::Column::NUMBER),
    Table::Column("token", Table::Column::TEXT),
    Table::Column("type", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("relations", Table::Column::TEXT)
  };

  const Table::Column CAMPAIGN_KEYWORD_TABLE_COLUMNS[] =
  {
    Table::Column("ccg_keyword_id", Table::Column::NUMBER),
    Table::Column("original_keyword", Table::Column::TEXT),
    Table::Column("click_url", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column SITES_TABLE_COLUMNS[] =
  {
    Table::Column("site_id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("noads_timeout", Table::Column::NUMBER),
    Table::Column("freq_cap_id", Table::Column::NUMBER),
    Table::Column("approved_creative_categories", Table::Column::TEXT),
    Table::Column("rejected_creative_categories", Table::Column::TEXT),
    Table::Column("approved_creatives", Table::Column::TEXT),
    Table::Column("rejected_creatives", Table::Column::TEXT),
    Table::Column("flags", Table::Column::NUMBER),
    Table::Column("account_id", Table::Column::NUMBER)
  };

  const Table::Column SITES_EXPANDED_TABLE_COLUMNS[] =
  {
    Table::Column("site_id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("noads_timeout", Table::Column::NUMBER),
    Table::Column("freq_cap_id", Table::Column::NUMBER),
    Table::Column("fc_lifelimit", Table::Column::NUMBER),
    Table::Column("fc_period", Table::Column::NUMBER),
    Table::Column("fc_window_limit", Table::Column::NUMBER),
    Table::Column("fc_window_time", Table::Column::NUMBER),
    Table::Column("approved_creative_categories", Table::Column::TEXT),
    Table::Column("rejected_creative_categories", Table::Column::TEXT),
    Table::Column("approved_creatives", Table::Column::TEXT),
    Table::Column("rejected_creatives", Table::Column::TEXT),
    Table::Column("flags", Table::Column::NUMBER),
    Table::Column("account_id", Table::Column::NUMBER)
  };

  const Table::Column CAMPAIGN_TABLE_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("account_id", Table::Column::NUMBER),
    Table::Column("advertiser_id", Table::Column::NUMBER),
    Table::Column("campaign_group_id", Table::Column::NUMBER),
    Table::Column("ccg_rate_id", Table::Column::NUMBER),
    Table::Column("ccg_rate_type", Table::Column::TEXT),
    Table::Column("fc_id", Table::Column::NUMBER),
    Table::Column("group_fc_id", Table::Column::NUMBER),
    Table::Column("weeklyrun", Table::Column::TEXT),
    Table::Column("ecpm", Table::Column::NUMBER),
    Table::Column("ctr", Table::Column::TEXT),
    Table::Column("channel", Table::Column::TEXT),
    Table::Column("stat_channel", Table::Column::TEXT),
    Table::Column("country", Table::Column::TEXT),
    Table::Column("sites", Table::Column::TEXT),
    Table::Column("creatives", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("eval_status", Table::Column::TEXT),
    Table::Column("campaign_type", Table::Column::TEXT),
    Table::Column("target_type", Table::Column::TEXT),
    Table::Column("start_user_group_id", Table::Column::NUMBER),
    Table::Column("end_user_group_id", Table::Column::NUMBER),
    Table::Column("ctr_reset_id", Table::Column::NUMBER),
    Table::Column("random_imps", Table::Column::NUMBER),
    Table::Column("mode", Table::Column::TEXT),
    Table::Column("min_uid_age", Table::Column::TEXT),
    Table::Column("seq_set_rotate_imps", Table::Column::NUMBER),

    Table::Column("date_start", Table::Column::TEXT),
    Table::Column("date_end", Table::Column::TEXT),
    Table::Column("budget", Table::Column::TEXT),
    Table::Column("daily_budget", Table::Column::TEXT),
    Table::Column("delivery_pacing", Table::Column::TEXT),
    Table::Column("imps", Table::Column::TEXT),
    Table::Column("daily_imps", Table::Column::TEXT),
    Table::Column("clicks", Table::Column::TEXT),
    Table::Column("daily_clicks", Table::Column::TEXT),

    Table::Column("cmp_date_start", Table::Column::TEXT),
    Table::Column("cmp_date_end", Table::Column::TEXT),
    Table::Column("cmp_budget", Table::Column::TEXT),
    Table::Column("cmp_daily_budget", Table::Column::TEXT),
    Table::Column("cmp_delivery_pacing", Table::Column::TEXT),
    Table::Column("cmp_imps", Table::Column::TEXT),
    Table::Column("cmp_daily_imps", Table::Column::TEXT),
    Table::Column("cmp_clicks", Table::Column::TEXT),
    Table::Column("cmp_daily_clicks", Table::Column::TEXT),

    Table::Column("max_pub_share", Table::Column::TEXT),
    Table::Column("bid_strategy", Table::Column::TEXT),
    Table::Column("min_ctr_goal", Table::Column::TEXT),
    Table::Column("commision", Table::Column::TEXT),
    Table::Column("flags", Table::Column::TEXT),
    Table::Column("marketplace", Table::Column::TEXT)
  };

  const Table::Column CAMPAIGN_EXPANDED_TABLE_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::NUMBER),
    Table::Column("account_id", Table::Column::NUMBER),
    Table::Column("account_flags", Table::Column::NUMBER),
    Table::Column("text_adserving", Table::Column::TEXT),
    Table::Column("account_currency_id", Table::Column::TEXT),
    Table::Column("account_timestamp", Table::Column::TEXT),
    Table::Column("advertiser_id", Table::Column::NUMBER),
    Table::Column("campaign_group_id", Table::Column::NUMBER),
    Table::Column("ccg_rate_id", Table::Column::NUMBER),
    Table::Column("ccg_rate_type", Table::Column::TEXT),
    Table::Column("fc_id", Table::Column::NUMBER),
    Table::Column("fc_lifelimit", Table::Column::NUMBER),
    Table::Column("fc_period", Table::Column::NUMBER),
    Table::Column("fc_window_limit", Table::Column::NUMBER),
    Table::Column("fc_window_time", Table::Column::NUMBER),
    Table::Column("group_fc_id", Table::Column::NUMBER),
    Table::Column("group_fc_lifelimit", Table::Column::NUMBER),
    Table::Column("group_fc_period", Table::Column::NUMBER),
    Table::Column("group_fc_window_limit", Table::Column::NUMBER),
    Table::Column("group_fc_window_time", Table::Column::NUMBER),
    Table::Column("weeklyrun", Table::Column::TEXT),
    Table::Column("ecpm", Table::Column::NUMBER),
    Table::Column("ctr", Table::Column::TEXT),
    Table::Column("channels", Table::Column::TEXT),
    Table::Column("stat_channel", Table::Column::TEXT),
    Table::Column("country", Table::Column::TEXT),
    Table::Column("sites", Table::Column::TEXT),
    Table::Column("creatives", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("eval_status", Table::Column::TEXT),
    Table::Column("campaign_type", Table::Column::TEXT),
    Table::Column("target_type", Table::Column::TEXT),
    Table::Column("start_user_group_id", Table::Column::NUMBER),
    Table::Column("end_user_group_id", Table::Column::NUMBER),
    Table::Column("ctr_reset_id", Table::Column::NUMBER),
    Table::Column("random_imps", Table::Column::NUMBER),
    Table::Column("mode", Table::Column::TEXT),
    Table::Column("min_uid_age", Table::Column::TEXT),
    Table::Column("seq_set_rotate_imps", Table::Column::NUMBER),

    Table::Column("date_start", Table::Column::TEXT),
    Table::Column("date_end", Table::Column::TEXT),
    Table::Column("budget", Table::Column::TEXT),
    Table::Column("daily_budget", Table::Column::TEXT),
    Table::Column("delivery_pacing", Table::Column::TEXT),
    Table::Column("imps", Table::Column::TEXT),
    Table::Column("daily_imps", Table::Column::TEXT),
    Table::Column("clicks", Table::Column::TEXT),
    Table::Column("daily_clicks", Table::Column::TEXT),

    Table::Column("cmp_date_start", Table::Column::TEXT),
    Table::Column("cmp_date_end", Table::Column::TEXT),
    Table::Column("cmp_budget", Table::Column::TEXT),
    Table::Column("cmp_daily_budget", Table::Column::TEXT),
    Table::Column("cmp_delivery_pacing", Table::Column::TEXT),
    Table::Column("cmp_imps", Table::Column::TEXT),
    Table::Column("cmp_daily_imps", Table::Column::TEXT),
    Table::Column("cmp_clicks", Table::Column::TEXT),
    Table::Column("cmp_daily_clicks", Table::Column::TEXT),

    Table::Column("max_pub_share", Table::Column::TEXT),
    Table::Column("bid_strategy", Table::Column::TEXT),
    Table::Column("min_ctr_goal", Table::Column::TEXT),
    Table::Column("commision", Table::Column::TEXT),
    Table::Column("flags", Table::Column::TEXT),
    Table::Column("marketplace", Table::Column::TEXT),

    Table::Column("colocations", Table::Column::TEXT),
    Table::Column("exclude_pub_accounts", Table::Column::TEXT),
    Table::Column("exclude_tags", Table::Column::TEXT),
    Table::Column("delivery_coef", Table::Column::NUMBER),
    Table::Column("imp_revenue", Table::Column::TEXT),
    Table::Column("click_revenue", Table::Column::TEXT),
    Table::Column("action_revenue", Table::Column::TEXT)
  };

  const Table::Column TAGS_TABLE_COLUMNS[] =
  {
    Table::Column("tag_id", Table::Column::NUMBER),
    Table::Column("site_id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),

    Table::Column("sizes", Table::Column::TEXT),

    Table::Column("imp_track_pixel", Table::Column::TEXT),
    Table::Column("passback", Table::Column::TEXT),
    Table::Column("passback_type", Table::Column::TEXT),
    Table::Column("flags", Table::Column::NUMBER),
    Table::Column("marketplace", Table::Column::TEXT),
    Table::Column("adjustment", Table::Column::TEXT),
    Table::Column("accepted_categories", Table::Column::TEXT),
    Table::Column("rejected_categories", Table::Column::TEXT),
    Table::Column("allow_expandable", Table::Column::NUMBER),
    Table::Column("auction_max_ecpm_share", Table::Column::TEXT),
    Table::Column("auction_prop_probability_share", Table::Column::TEXT),
    Table::Column("auction_random_share", Table::Column::TEXT),
    Table::Column("cost_coef", Table::Column::TEXT),
    Table::Column("tag_pricings_timestamp", Table::Column::TEXT),
    Table::Column("tag_pricings", Table::Column::TEXT),
    Table::Column("hidden_options", Table::Column::TEXT),
    Table::Column("passback_options", Table::Column::TEXT),
    Table::Column("options", Table::Column::TEXT),
  };

  const Table::Column CURRENCY_TABLE_COLUMNS[] =
  {
    Table::Column("currency_id", Table::Column::NUMBER),
    Table::Column("currency_exchange_id", Table::Column::NUMBER),
    Table::Column("effective_date", Table::Column::TEXT),
    Table::Column("rate", Table::Column::TEXT),
    Table::Column("fraction_digits", Table::Column::NUMBER),
    Table::Column("currency_code", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column CREATIVE_TABLE_COLUMNS[] =
  {
    Table::Column("ccid", Table::Column::NUMBER),
    Table::Column("creative_id", Table::Column::NUMBER),
    Table::Column("campaign_id", Table::Column::NUMBER),
    Table::Column("fc_id", Table::Column::NUMBER),
    Table::Column("weight", Table::Column::NUMBER),
    Table::Column("creative_format", Table::Column::TEXT),
    Table::Column("click_url", Table::Column::TEXT),
    Table::Column("sizes", Table::Column::TEXT),
    Table::Column("version_id", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("categories", Table::Column::TEXT),
    Table::Column("order_set_id", Table::Column::NUMBER),
    Table::Column("initial_contract_id", Table::Column::NUMBER),
    Table::Column("options", Table::Column::TEXT)
  };

  const Table::Column CREATIVE_EXPANDED_TABLE_COLUMNS[] =
  {
    Table::Column("ccid", Table::Column::NUMBER),
    Table::Column("creative_id", Table::Column::NUMBER),
    Table::Column("campaign_id", Table::Column::NUMBER),
    Table::Column("campaign_group_id", Table::Column::NUMBER),
    Table::Column("fc_id", Table::Column::NUMBER),
    Table::Column("fc_lifelimit", Table::Column::NUMBER),
    Table::Column("fc_period", Table::Column::NUMBER),
    Table::Column("fc_window_limit", Table::Column::NUMBER),
    Table::Column("fc_window_time", Table::Column::NUMBER),
    Table::Column("weight", Table::Column::NUMBER),
    Table::Column("creative_format", Table::Column::TEXT),
    Table::Column("click_url", Table::Column::TEXT),
    Table::Column("creative_size", Table::Column::TEXT),
    Table::Column("version_id", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("categories", Table::Column::TEXT),
    Table::Column("order_set_id", Table::Column::NUMBER),
    Table::Column("initial_contract_id", Table::Column::NUMBER),

    Table::Column("campaign_fc_id", Table::Column::NUMBER),
    Table::Column("campaign_fc_lifelimit", Table::Column::NUMBER),
    Table::Column("campaign_fc_period", Table::Column::NUMBER),
    Table::Column("campaign_fc_window_limit", Table::Column::NUMBER),
    Table::Column("campaign_fc_window_time", Table::Column::NUMBER),
    Table::Column("campaign_group_fc_id", Table::Column::NUMBER),
    Table::Column("campaign_group_fc_lifelimit", Table::Column::NUMBER),
    Table::Column("campaign_group_fc_period", Table::Column::NUMBER),
    Table::Column("campaign_group_fc_window_limit", Table::Column::NUMBER),
    Table::Column("campaign_group_fc_window_time", Table::Column::NUMBER),

    Table::Column("weeklyrun", Table::Column::TEXT),
    Table::Column("campaign_ecpm", Table::Column::NUMBER),
    Table::Column("campaign_ctr", Table::Column::TEXT),
    Table::Column("campaign_flags", Table::Column::TEXT),
    Table::Column("campaign_country", Table::Column::TEXT),
    Table::Column("campaign_sites", Table::Column::TEXT),
    Table::Column("campaign_creatives", Table::Column::TEXT),
    Table::Column("campaign_status", Table::Column::TEXT),

    Table::Column("options", Table::Column::TEXT)
  };

  const Table::Column COLOCATIONS_TABLE_COLUMNS[] =
  {
    Table::Column("colo_name", Table::Column::TEXT),
    Table::Column("colo_id", Table::Column::NUMBER),
    Table::Column("colo_rate_id", Table::Column::NUMBER),
    Table::Column("account_id", Table::Column::NUMBER),
    Table::Column("revenue_share", Table::Column::TEXT),
    Table::Column("ad_serving", Table::Column::TEXT),
    Table::Column("hid_profile", Table::Column::NUMBER),
    Table::Column("options", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };
  
  const Table::Column COUNTRIES_TABLE_COLUMNS[] =
  {
    Table::Column("country_code", Table::Column::TEXT),
    Table::Column("options", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column GLOBAL_TABLE_COLUMNS[] =
  {
    Table::Column("currency_exchange_id", Table::Column::NUMBER),
    Table::Column("fraud_user_deactivate_period", Table::Column::NUMBER),
    Table::Column("cost_limit", Table::Column::TEXT),
    Table::Column("google_publisher_account_id", Table::Column::NUMBER),
    Table::Column("global_params_timestamp", Table::Column::TEXT),
    Table::Column("master_stamp", Table::Column::TEXT),
    Table::Column("first_load_stamp", Table::Column::TEXT),
    Table::Column("finish_load_stamp", Table::Column::TEXT)
  };

  const Table::Column FREQ_CAPS_TABLE_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("lifelimit", Table::Column::NUMBER),
    Table::Column("period", Table::Column::NUMBER),
    Table::Column("window_limit", Table::Column::NUMBER),
    Table::Column("window_time", Table::Column::NUMBER)
  };

  const Table::Column CREATIVE_TEMPLATES_TABLE_COLUMNS[] =
  {
    Table::Column("creative_format", Table::Column::TEXT),
    Table::Column("creative_size", Table::Column::TEXT),
    Table::Column("app_format", Table::Column::TEXT),
    Table::Column("mime_format", Table::Column::TEXT),
    Table::Column("track_impression", Table::Column::NUMBER),
    Table::Column("template_file", Table::Column::TEXT),
    Table::Column("type", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("tokens", Table::Column::TEXT),
    Table::Column("hidden_tokens", Table::Column::TEXT),
  };

  const Table::Column CREATIVE_CATEGORY_TABLE_COLUMNS[] =
  {
    Table::Column("creative_category_id", Table::Column::NUMBER),
    Table::Column("cct_id", Table::Column::NUMBER),
    Table::Column("name", Table::Column::TEXT),
    Table::Column("external_categories", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column ADV_ACTION_TABLE_COLUMNS[] =
  {
    Table::Column("action_id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("cur_value", Table::Column::TEXT),
    Table::Column("ccg_ids", Table::Column::TEXT)
  };

  const Table::Column CATEGORY_CHANNEL_TABLE_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("name", Table::Column::TEXT),
    Table::Column("newsgate_name", Table::Column::TEXT),
    Table::Column("parent_channel_id", Table::Column::NUMBER),
    Table::Column("flags", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("localizations", Table::Column::TEXT)
  };

  const Table::Column GEO_CHANNEL_TABLE_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("country", Table::Column::TEXT),
    Table::Column("region", Table::Column::TEXT),
    Table::Column("city", Table::Column::TEXT)
  };

  const Table::Column GEO_COORD_CHANNEL_TABLE_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("longitude", Table::Column::NUMBER),
    Table::Column("latitude", Table::Column::NUMBER),
    Table::Column("radius", Table::Column::NUMBER)
  };

  const Table::Column WEB_APPLICATIONS_TABLE_COLUMNS[] =
  {
    Table::Column("web_operation_id", Table::Column::NUMBER),
    Table::Column("app", Table::Column::TEXT),
    Table::Column("source", Table::Column::TEXT),
    Table::Column("operation", Table::Column::TEXT),
    Table::Column("flags", Table::Column::NUMBER)
  };

  const Table::Column SIZE_TABLE_COLUMNS[] =
  {
    Table::Column("size_id", Table::Column::NUMBER),
    Table::Column("size_type_id", Table::Column::NUMBER),
    Table::Column("protocol_name", Table::Column::TEXT),
    Table::Column("width", Table::Column::NUMBER),
    Table::Column("height", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column CONTRACT_TABLE_COLUMNS[] =
  {
    Table::Column("number", Table::Column::TEXT),
    Table::Column("date", Table::Column::TEXT),
    Table::Column("type", Table::Column::TEXT),
    Table::Column("vat_included", Table::Column::NUMBER),
    Table::Column("ord_contract_id", Table::Column::TEXT),
    Table::Column("ord_ado_id", Table::Column::TEXT),
    Table::Column("subject_type", Table::Column::TEXT),
    Table::Column("action_type", Table::Column::TEXT),
    Table::Column("agent_acting_for_publisher", Table::Column::NUMBER),
    Table::Column("parent_contract_id", Table::Column::NUMBER),
    Table::Column("client_id", Table::Column::TEXT),
    Table::Column("client_name", Table::Column::TEXT),
    Table::Column("client_legal_form", Table::Column::TEXT),
    Table::Column("contractor_id", Table::Column::TEXT),
    Table::Column("contractor_name", Table::Column::TEXT),
    Table::Column("contractor_legal_form", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT)
  };
}

////////////////////////////////////////////////////
// class Application
////////////////////////////////////////////////////

std::string
weekly_run_convert(unsigned long weekly_run)
{
  std::ostringstream ostr;
  Generics::Time time_val(Generics::Time::ONE_MINUTE * weekly_run);
  ostr << (time_val / Generics::Time::ONE_DAY.tv_sec).tv_sec << ":" <<
    time_val.get_gm_time().format("%H.%M");
  return ostr.str();
}

template<typename SeqType>
std::string
interval_seq_to_string(const SeqType& seq)
{
  std::ostringstream ostr;
  for(CORBA::ULong i = 0; i < seq.length(); ++i)
  {
    if(i != 0) ostr << " ";
    ostr << "[ " << seq[i].min << ", " << seq[i].max << "]";
  }
  return ostr.str();
}

std::string
optional_decimal_to_string(
  const CORBACommons::OptionalDecimalInfo& optional_decimal_info)
{
  AdServer::CampaignSvcs::OptionalRevenueDecimal revenue =
    CorbaAlgs::unpack_optional_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(optional_decimal_info);
  return revenue ? revenue->str() : "null";
}

std::string
ccg_type_to_string(AdServer::CampaignSvcs::CCGType ccg_type)
{
  using namespace AdServer::CampaignSvcs;

  if(ccg_type == CT_DISPLAY)
  {
    return "display";
  }
  else if(ccg_type == CT_TEXT)
  {
    return "text";
  }
  else if(ccg_type == CT_ALL)
  {
    return "all";
  }

  return "unk";
}

std::string
ccg_rate_type_to_string(AdServer::CampaignSvcs::CCGRateType ccg_rate_type)
{
  using namespace AdServer::CampaignSvcs;

  if(ccg_rate_type == CR_CPM)
  {
    return "cpm";
  }
  else if(ccg_rate_type == CR_CPC)
  {
    return "cpc";
  }
  else if(ccg_rate_type == CR_CPA)
  {
    return "cpa";
  }
  else if(ccg_rate_type == CR_MAXBID)
  {
    return "max bid";
  }
  else if(ccg_rate_type == CR_ALL)
  {
    return "all";
  }

  return "unk";
}

std::string
direction_set_to_string(unsigned long directions)
{
  using namespace AdServer::CampaignSvcs;

  std::string res;
  if(directions & ED_UP)
  {
    res += (res.empty() ? std::string() : std::string(", ")) + "up";
  }
  if(directions & ED_RIGHT)
  {
    res += (res.empty() ? std::string() : std::string(", ")) + "right";
  }
  if(directions & ED_DOWN)
  {
    res += (res.empty() ? std::string() : std::string(", ")) + "down";
  }
  if(directions & ED_LEFT)
  {
    res += (res.empty() ? std::string() : std::string(", ")) + "left";
  }

  return res;
}

std::string
campaign_mode_to_string(AdServer::CampaignSvcs::CampaignMode campaign_mode)
{
  using namespace AdServer::CampaignSvcs;

  if(campaign_mode == CM_RANDOM)
  {
    return "random";
  }
  else if(campaign_mode == CM_NON_RANDOM)
  {
    return "normal";
  }

  return "invalid !!!";
}

/*
std::string
campaign_contracts_to_string(const AdServer::CampaignSvcs::CampaignContractSeq& contracts)
{
  using namespace AdServer::CampaignSvcs;

  std::ostringstream ostr;
  for(CORBA::ULong contract_i = 0; contract_i < contracts.length(); ++contract_i)
  {
    const AdServer::CampaignSvcs::CampaignContractInfo& contract = contracts[contract_i];

    ostr << (contract_i != 0 ? " " : "") << "[" <<
      "ord_contract_id = " << contract.ord_contract_id <<
      ", ord_ado_id = " << contract.ord_ado_id <<
      ", id = " << contract.id <<
      ", date = " << contract.date <<
      ", type = " << contract.type <<
      ", client_id = " << contract.client_id <<
      ", client_name = " << contract.client_name <<
      ", contractor_id = " << contract.contractor_id <<
      ", contractor_name = " << contract.contractor_name <<
      "]";
  }

  return ostr.str();
}
*/

template<typename IntType>
std::string
int_to_str(const IntType& val)
{
  std::ostringstream ostr;
  ostr << val;
  return ostr.str();
}

int
main(int argc, char** argv)
{
  Application* app = 0;

  try
  {
    app = Application::instance();
    return app->run(argc, argv);
  }
  catch(const Application::InvalidArgument& e)
  {
    std::cerr << "Invalid argument. Exception caught:\n" <<
      e.what() << "\nRun 'CampaignAdmin help' for usage details\n";
  }
  catch(const AdServer::CampaignSvcs::CampaignManager::
        ImplementationException& e)
  {
    std::cerr << "CampaignAdmin: CampaignManager::ImplementationException "
      "exception caught:" << std::endl << e.description.in() <<
      std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "CampaignAdmin: eh::Exception exception caught:" <<
      std::endl << e << std::endl;
  }
  catch(const eh::Exception& e)
  {
    std::cerr << "CampaignAdmin: eh::Exception exception caught:" <<
      std::endl << e.what() << std::endl;
  }

  return -1;
}

namespace
{
  template <typename Option, const size_t SIZE>
  void
  args_add(Generics::AppUtils::Args& args, Option (&options)[SIZE])
    /*throw(eh::Exception)*/
  {
    for (size_t i = 0; i < SIZE; i++)
    {
      args.add(Generics::AppUtils::equal_name(options[i].equal_name) ||
        Generics::AppUtils::short_name(options[i].short_name),
        options[i].value);
    }
  }
}

Application::Application() /*throw(Application::Exception, eh::Exception)*/
  : corba_client_adapter_(new CORBACommons::CorbaClientAdapter())

{}

Application::~Application() noexcept
{}

int
Application::parse_args_(int argc, char** argv) /*throw(InvalidArgument)*/
{
  try
  {
    Generics::AppUtils::Args args(-1);
    args_add(args, check_options);
    args_add(args, string_options);
    args_add(args, ulong_options);
    args.parse(argc - 1, argv + 1);
    int c_index = command_max;
    for(Generics::AppUtils::Args::CommandList::const_iterator com_it =
      args.commands().begin(); com_it != args.commands().end(); ++com_it)
    {
      const std::string& com = *com_it;
      if(c_index == command_max)
      {
        for (size_t i = 0; i < command_max; i++)
        {
          if (com == COMMAND[i])
          {
            c_index = i;
            break;
          }
        }
        if(c_index != command_max)
        {
          continue;
        }
      }
      std::string name, value;
      Table::Relation rel;
      if (Table::parse_filter(com.c_str(), name, rel, value))
      {
        filters_.push_back(Table::Filter(name.c_str(), value.c_str(), rel));
      }
      else
      {
        Stream::Error err;
        err << "Unknown command '" << com << "'";
        throw InvalidArgument(err);
      }
    }
    if (c_index == command_max)
    {
      c_index = command_help;
    }
    else if(c_index != command_decode && c_index != command_help)
    {
      if(!string_options[OPT_REFERENCE].value.installed())
      {
        if(!string_options[OPT_HOST_AND_PORT].value.installed())
        {
          throw InvalidArgument("'reference' option didn't set");
        }
        else
        {
          std::string ref = "corbaloc:iiop:";
          ref += *string_options[OPT_HOST_AND_PORT].value;
          if(c_index == command_expression_channel ||
             c_index == command_behave_keys ||
             c_index == command_behave_param ||
             c_index == command_simple_channel ||
             c_index == command_simple_channel_cs ||
             c_index == command_search_engines ||
             c_index == command_detectors ||
             c_index == command_fraud_condition ||
             c_index == command_stat ||
             c_index == command_stat_ccg ||
             c_index == command_stat_account ||
             c_index == command_stat_campaign ||
             c_index == command_update_stat ||
             c_index == command_bill_accounts ||
             c_index == command_bill_campaigns ||
             c_index == command_bill_ccgs)
          {
            ref += "/CampaignServer";
          }
          else
          {
            ref += "/CampaignManager";
          }
          string_options[OPT_REFERENCE].value.set("", ref.c_str());
        }
      }
      std::string name, value;
      Table::Relation rel;
      if (string_options[OPT_FILTER].value.installed())
      {
        if (Table::parse_filter(
            string_options[OPT_FILTER].value->c_str(), name, rel, value))
        {
          filters_.push_back(Table::Filter(name.c_str(), value.c_str(), rel));
        }
      }
      if (string_options[OPT_SORTER].value.installed())
      {
        if (Table::parse_filter(
            string_options[OPT_SORTER].value->c_str(), name, rel, value))
        {
          sorter_ = Table::Sorter(value.c_str(), rel == Table::RL_LT);
        }
      }
    }
    return c_index;
  }
  catch(const eh::Exception& e)
  {
    throw InvalidArgument(e.what());
  }
}

void Application::init_manager_ref_() /*throw(InvalidArgument)*/
{
  const std::string& manager_ref = *string_options[OPT_REFERENCE].value;
  try
  {
    CORBA::Object_var obj = corba_client_adapter_->resolve_object(
      CORBACommons::CorbaObjectRef(manager_ref.c_str()));

    manager_ = AdServer::CampaignSvcs::CampaignManager::_narrow(obj.in());
  }
  catch(const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << "failed to resolve manager reference '" <<
      manager_ref.c_str() << "'. eh::Exception caught: " << ex.what();
    throw InvalidArgument(ostr);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << "failed to resolve manager reference '" <<
      manager_ref.c_str() << "'. CORBA::SystemException caught: " << e;
    throw InvalidArgument(ostr);
  }

  if(CORBA::is_nil(manager_))
  {
    Stream::Error ostr;
    ostr << "AdServer::CampaignSvcs::CampaignManager::_narrow failed "
      "for manager reference '" << manager_ref.c_str() << "'";
    throw InvalidArgument(ostr);
  }
}

void Application::init_server_ref_() /*throw(InvalidArgument)*/
{
  const std::string& server_ref = *string_options[OPT_REFERENCE].value;
  try
  {
    CORBA::Object_var obj = corba_client_adapter_->resolve_object(
      CORBACommons::CorbaObjectRef(server_ref.c_str()));

    server_ = AdServer::CampaignSvcs::CampaignServer::_narrow(obj.in());
  }
  catch(const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << "failed to resolve server reference '" <<
      server_ref.c_str() << "'. eh::Exception caught: " << ex.what();
    throw InvalidArgument(ostr);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << "failed to resolve server reference '" <<
      server_ref.c_str() << "'. CORBA::SystemException caught: " << e;

    throw InvalidArgument(ostr);
  }

  if(CORBA::is_nil(server_))
  {
    Stream::Error ostr;
    ostr << "AdServer::CampaignSvcs::CampaignServer::_narrow failed "
      "for server reference '" << server_ref.c_str() << "'";
    throw InvalidArgument(ostr);
  }
}

int
Application::run(int& argc, char** argv)
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  if (argc < 2)
  {
    throw InvalidArgument("Too few arguments");
  }
  int command_index = parse_args_(argc, argv);
  switch(command_index)
  {
    case command_help:
      return help();
    case command_app_format:
    case command_globals:
    case command_creative_categories:
    case command_sites:
    case command_colocations:
    case command_countries:
    case command_creative_templates:
    case command_campaign:
    case command_tags:
    case command_currency:
    case command_creative:
    case command_freq_caps:
    case command_creative_option:
    case command_adv_action:
    case command_category_channel:
    case command_geo_channel:
    case command_geo_coord_channel:
    case command_web_operations:
    case command_keyword:
    case command_account:
    case command_size:
    case command_contract:
      return show(command_index);
      break;
    case command_fraud_condition:
      fraud_conditions();
      break;
    case command_select:
      select();
      break;
    case command_preview:
      preview();
      break;
    case command_expression_channel:
      expression_channels();
      break;
    case command_simple_channel:
      simple_channel();
      break;
    case command_simple_channel_cs:
      simple_channel_cs();
      break;
    case command_trace_index:
      return trace_campaign_index();
      break;
    case command_trace_weight:
      return trace_campaign_weight();
      break;
    case command_behave_param:
      behav_param(command_index);
      break;
    case command_behave_keys:
      behav_param(command_index);
      break;
    case command_search_engines:
      search_engines(
        check_options[OPT_UPDATE].value.enabled(),
        check_options[OPT_DELETED].value.enabled());
      break;
    case command_colocation_flags:
      colocation_flags();
      break;
    case command_detectors:
      detectors(
        check_options[OPT_WEB].value.enabled(),
        check_options[OPT_PLATFORM].value.enabled());
      break;
    case command_update_stat:
      update_stat();
      break;
    case command_stat:
    case command_stat_ccg:
    case command_stat_account:
      stat(command_index == command_stat_account);
      break;
    case command_stat_campaign:
      campaign_stat();
      break;
    case command_decode:
      return decode();
      break;
    case command_bill_accounts:
    case command_bill_campaigns:
    case command_bill_ccgs:
      return bill_stats(command_index);
      break;
    default:
      throw InvalidArgument("Invalid command");
      break;
  }

  return 0;
}

int
Application::help() noexcept
{
  std::cerr << USAGE;
  return 0;
}

int
Application::show(int aspect)
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  init_manager_ref_();
  AdServer::CampaignSvcs::CampaignManager::GetConfigInfo get_config_props;
  get_config_props.geo_channels = (
    aspect == command_geo_channel || aspect == command_geo_coord_channel);

  AdServer::CampaignSvcs::CampaignManager::CampaignConfig_var config =
    manager_->get_config(get_config_props);

  bool expand = check_options[OPT_EXPAND].value.enabled();
  bool expand_options = check_options[OPT_EXPAND_OPT].value.enabled();

  unsigned long columns = 0;
  const Table::Column* columns_ptr = 0;
  switch(aspect)
  {
    case command_globals:
      columns_ptr = GLOBAL_TABLE_COLUMNS;
      columns = sizeof(GLOBAL_TABLE_COLUMNS) / sizeof(GLOBAL_TABLE_COLUMNS[0]);
      break;
    case command_account:
      columns_ptr = ACCOUNT_TABLE_COLUMNS;
      columns =
        sizeof(ACCOUNT_TABLE_COLUMNS) / sizeof(ACCOUNT_TABLE_COLUMNS[0]);
      break;
    case command_creative_categories:
      columns_ptr = CREATIVE_CATEGORY_TABLE_COLUMNS;
      columns = sizeof(CREATIVE_CATEGORY_TABLE_COLUMNS) /
        sizeof(CREATIVE_CATEGORY_TABLE_COLUMNS[0]);
      break;
    case command_sites:
      if(expand)
      {
        columns_ptr = SITES_EXPANDED_TABLE_COLUMNS;
        columns = sizeof(SITES_EXPANDED_TABLE_COLUMNS) /
          sizeof(SITES_EXPANDED_TABLE_COLUMNS[0]);
      }
      else
      {
        columns_ptr = SITES_TABLE_COLUMNS;
        columns =
          sizeof(SITES_TABLE_COLUMNS) / sizeof(SITES_TABLE_COLUMNS[0]);
      }
      break;
    case command_colocations:
      columns_ptr = COLOCATIONS_TABLE_COLUMNS;
      columns =
        sizeof(COLOCATIONS_TABLE_COLUMNS) / sizeof(COLOCATIONS_TABLE_COLUMNS[0]);
      break;
    case command_countries:
        columns_ptr = COUNTRIES_TABLE_COLUMNS;
      columns =
        sizeof(COUNTRIES_TABLE_COLUMNS) / sizeof(COUNTRIES_TABLE_COLUMNS[0]);
      break;
    case command_creative_templates:
      columns_ptr = CREATIVE_TEMPLATES_TABLE_COLUMNS;
      columns = sizeof(CREATIVE_TEMPLATES_TABLE_COLUMNS) /
        sizeof(CREATIVE_TEMPLATES_TABLE_COLUMNS[0]);
      break;
    case command_campaign:
      if(expand)
      {
        columns_ptr = CAMPAIGN_EXPANDED_TABLE_COLUMNS;
        columns = sizeof(CAMPAIGN_EXPANDED_TABLE_COLUMNS) /
          sizeof(CAMPAIGN_EXPANDED_TABLE_COLUMNS[0]);
      }
      else
      {
        columns_ptr = CAMPAIGN_TABLE_COLUMNS;
        columns = sizeof(CAMPAIGN_TABLE_COLUMNS) /
          sizeof(CAMPAIGN_TABLE_COLUMNS[0]);
      }
      break;
    case command_tags:
      columns_ptr = TAGS_TABLE_COLUMNS;
      columns = sizeof(TAGS_TABLE_COLUMNS) / sizeof(TAGS_TABLE_COLUMNS[0]);
      break;
    case command_currency:
      columns_ptr = CURRENCY_TABLE_COLUMNS;
      columns = sizeof(CURRENCY_TABLE_COLUMNS) /
        sizeof(CURRENCY_TABLE_COLUMNS[0]);
      break;
    case command_creative:
      if(expand)
      {
        columns_ptr = CREATIVE_EXPANDED_TABLE_COLUMNS;
        columns = sizeof(CREATIVE_EXPANDED_TABLE_COLUMNS) /
          sizeof(CREATIVE_EXPANDED_TABLE_COLUMNS[0]);
      }
      else
      {
        columns_ptr = CREATIVE_TABLE_COLUMNS;
        columns = sizeof(CREATIVE_TABLE_COLUMNS) /
          sizeof(CREATIVE_TABLE_COLUMNS[0]);
      }
      break;
    case command_freq_caps:
      columns_ptr = FREQ_CAPS_TABLE_COLUMNS;
      columns = sizeof(FREQ_CAPS_TABLE_COLUMNS) /
        sizeof(FREQ_CAPS_TABLE_COLUMNS[0]);
      break;
    case command_keyword:
      columns_ptr = CAMPAIGN_KEYWORD_TABLE_COLUMNS;
      columns = sizeof(CAMPAIGN_KEYWORD_TABLE_COLUMNS) /
        sizeof(CAMPAIGN_KEYWORD_TABLE_COLUMNS[0]);
      break;
    case command_creative_option:
      columns_ptr = CREATIVE_OPTION_TABLE_COLUMNS;
      columns = sizeof(CREATIVE_OPTION_TABLE_COLUMNS) /
        sizeof(CREATIVE_OPTION_TABLE_COLUMNS[0]);
      break;
    case command_adv_action:
      columns_ptr = ADV_ACTION_TABLE_COLUMNS;
      columns = sizeof(ADV_ACTION_TABLE_COLUMNS) /
        sizeof(ADV_ACTION_TABLE_COLUMNS[0]);
      break;
    case command_category_channel:
      columns_ptr = CATEGORY_CHANNEL_TABLE_COLUMNS;
      columns = sizeof(CATEGORY_CHANNEL_TABLE_COLUMNS) /
        sizeof(CATEGORY_CHANNEL_TABLE_COLUMNS[0]);
      break;
    case command_geo_channel:
      columns_ptr = GEO_CHANNEL_TABLE_COLUMNS;
      columns = sizeof(GEO_CHANNEL_TABLE_COLUMNS) /
        sizeof(GEO_CHANNEL_TABLE_COLUMNS[0]);
      break;
    case command_geo_coord_channel:
      columns_ptr = GEO_COORD_CHANNEL_TABLE_COLUMNS;
      columns = sizeof(GEO_COORD_CHANNEL_TABLE_COLUMNS) /
        sizeof(GEO_COORD_CHANNEL_TABLE_COLUMNS[0]);
      break;
    case command_web_operations:
      columns_ptr = WEB_APPLICATIONS_TABLE_COLUMNS;
      columns = sizeof(WEB_APPLICATIONS_TABLE_COLUMNS) /
        sizeof(WEB_APPLICATIONS_TABLE_COLUMNS[0]);
      break;
    case command_app_format:
      columns_ptr = APP_FORMAT_COLUMNS;
      columns = sizeof(APP_FORMAT_COLUMNS) /
        sizeof(APP_FORMAT_COLUMNS[0]);
      break;
    case command_size:
      columns_ptr = SIZE_TABLE_COLUMNS;
      columns = sizeof(SIZE_TABLE_COLUMNS) / sizeof(SIZE_TABLE_COLUMNS[0]);
      break;
    case command_contract:
      columns_ptr = CONTRACT_TABLE_COLUMNS;
      columns = sizeof(CONTRACT_TABLE_COLUMNS) / sizeof(CONTRACT_TABLE_COLUMNS[0]);
      break;
  }
  assert(columns_ptr);

  TablePtr table = TablePtr(new Table(columns));

  for(unsigned long i = 0; i < columns; i++)
  {
    table->column(i, columns_ptr[i]);
  }


  if(aspect == command_globals)
  {
    Generics::Time timestamp(
      CorbaAlgs::unpack_time(config->master_stamp));
    Generics::Time first_load_ts(
      CorbaAlgs::unpack_time(config->first_load_stamp));
    Generics::Time g_ts(
      CorbaAlgs::unpack_time(config->global_params_timestamp));
    Table::Row row(table->columns());

    row.add_field(config->currency_exchange_id);
    row.add_field(CorbaAlgs::unpack_time(
      config->fraud_user_deactivate_period).tv_sec);
    row.add_field(CorbaAlgs::unpack_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(
        config->cost_limit).str());
    row.add_field(config->google_publisher_account_id);
    row.add_field(g_ts.get_gm_time().format(TIME_FORMAT));
    row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));
    row.add_field(first_load_ts.get_gm_time().format(TIME_FORMAT));
    row.add_field(CorbaAlgs::unpack_time(
      config->finish_load_stamp).get_gm_time().format(TIME_FORMAT));

    table->add_row(row, filters_, sorter_);
  }
  else if (aspect == command_tags)
  {
    std::string prefix;

    AdminTools::create_prefix(
      prefix,
      TAGS_TABLE_COLUMNS,
      table->columns());

    SizeMap sizes;
    fill_sizes_(sizes, config);

    for(unsigned long i = 0; i < config->tags.length(); i++)
    {
      const AdServer::CampaignSvcs::CampaignManager::AdaptedTagInfo& tag_info =
        config->tags[i];

      Table::Row row(table->columns());
      describe_tag(config, sizes, row, tag_info, prefix.c_str(), expand_options);
      table->add_row(row, filters_, sorter_);
    }
  }
  else if (aspect == command_currency)
  {
    for (CORBA::ULong i = 0; i < config->currencies.length(); i++)
    {
      const AdServer::CampaignSvcs::CurrencyInfo& currency_info =
        config->currencies[i];

      Generics::Time timestamp(
        CorbaAlgs::unpack_time(currency_info.timestamp));
      Table::Row row(table->columns());

      row.add_field(currency_info.currency_id);
      row.add_field(currency_info.currency_exchange_id);
      Generics::Time effective_date(currency_info.effective_date);
      row.add_field(effective_date.get_gm_time().format("%F"));
      row.add_field(CorbaAlgs::unpack_decimal<
        AdServer::CampaignSvcs::RevenueDecimal>(currency_info.rate).str());
      row.add_field(currency_info.fraction_digits);
      row.add_field(currency_info.currency_code);
      row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));

      table->add_row(row, filters_, sorter_);
    }
  }
  else if (aspect == command_account)
  {
    for(unsigned long i = 0; i < config->accounts.length(); i++)
    {
      const AdServer::CampaignSvcs::AccountInfo& acc_info =
        config->accounts[i];

      Table::Row row(table->columns());
      describe_account(config, row, acc_info);

      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_creative_categories)
  {
    for(unsigned long i = 0; i < config->creative_categories.length(); i++)
    {
      const AdServer::CampaignSvcs::CreativeCategoryInfo& ccat_info =
        config->creative_categories[i];

      Table::Row row(table->columns());
      describe_creative_category(config, row, ccat_info);

      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_sites)
  {
    for(unsigned long i = 0; i < config->sites.length(); i++)
    {
      const AdServer::CampaignSvcs::SiteInfo& site_info =
        config->sites[i];

      Table::Row row(table->columns());
      describe_site(config, row, site_info, expand);

      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_colocations)
  {
    std::string prefix;

    AdminTools::create_prefix(
      prefix,
      COLOCATIONS_TABLE_COLUMNS,
      table->columns());

    for(unsigned long i = 0; i < config->colocations.length(); i++)
    {
      const AdServer::CampaignSvcs::ColocationInfo& colocation_info =
        config->colocations[i];

      Table::Row row(table->columns());
      describe_colocation(config, row, colocation_info, prefix.c_str(), expand_options);
      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_countries)
  {
    std::string prefix;

    AdminTools::create_prefix(
      prefix,
      COUNTRIES_TABLE_COLUMNS,
      table->columns());

    for(unsigned long i = 0; i < config->countries.length(); i++)
    {
      const AdServer::CampaignSvcs::CountryInfo& country_info =
        config->countries[i];

      Table::Row row(table->columns());
      describe_country(config, row, country_info, prefix.c_str(), expand_options);
      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_freq_caps)
  {
    for(unsigned long i = 0; i < config->frequency_caps.length(); i++)
    {
      const AdServer::CampaignSvcs::FreqCapInfo& freq_cap_info =
        config->frequency_caps[i];

      Table::Row row(table->columns());
      Generics::Time timestamp(
        CorbaAlgs::unpack_time(freq_cap_info.timestamp));

      row.add_field(freq_cap_info.fc_id);
      row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));
      row.add_field(freq_cap_info.lifelimit);
      row.add_field(freq_cap_info.period);
      row.add_field(freq_cap_info.window_limit);
      row.add_field(freq_cap_info.window_time);

      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_creative_templates)
  {
    std::string prefix;

    AdminTools::create_prefix(
      prefix,
      CREATIVE_TEMPLATES_TABLE_COLUMNS,
      table->columns());

    for(unsigned long i = 0; i < config->creative_template_files.length(); i++)
    {
      const AdServer::CampaignSvcs::
      CampaignManager::CreativeTemplateFileInfo& cr_templ_info =
        config->creative_template_files[i];

      Table::Row row(table->columns());

      row.add_field(cr_templ_info.creative_format);
      row.add_field(cr_templ_info.creative_size);
      row.add_field(cr_templ_info.app_format);
      row.add_field(cr_templ_info.mime_format);
      row.add_field(cr_templ_info.track_impr);
      row.add_field(cr_templ_info.template_file);

      std::string str_type;
      if(cr_templ_info.type == AdServer::CampaignSvcs::CTT_TEXT)
      {
        str_type = "Text";
      }
      else if(cr_templ_info.type == AdServer::CampaignSvcs::CTT_XSLT)
      {
        str_type = "Xslt";
      }
      else
      {
        str_type = "Unknown";
      }

      row.add_field(str_type);

      Generics::Time timestamp(CorbaAlgs::unpack_time(cr_templ_info.timestamp));
      row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));
      row.add_field(cr_templ_info.status);

      std::string options_str;
      print_custom_options(config,
        options_str, cr_templ_info.tokens, prefix.c_str(), expand_options);
      row.add_field(options_str);

      std::string hidden_options_str;
      print_custom_options(config,
        hidden_options_str, cr_templ_info.hidden_tokens, prefix.c_str(), expand_options);
      row.add_field(hidden_options_str);

      table->add_row(row, filters_, sorter_);
    }
  }
  else if (aspect == command_keyword)
  {
    for(unsigned long i = 0; i < config->campaign_keywords.length(); i++)
    {
      const AdServer::CampaignSvcs::CampaignKeywordInfo& kw_info =
        config->campaign_keywords[i];

      Generics::Time timestamp(CorbaAlgs::unpack_time(kw_info.timestamp));
      Table::Row row(table->columns());

      row.add_field(kw_info.ccg_keyword_id);
      row.add_field(kw_info.original_keyword);
      row.add_field(kw_info.click_url);
      row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));

      table->add_row(row, filters_, sorter_);
    }
  }
  else if (aspect == command_creative_option)
  {
    for(unsigned long i = 0; i < config->creative_options.length(); i++)
    {
      const AdServer::CampaignSvcs::CreativeOptionInfo& co_info =
        config->creative_options[i];

      Generics::Time timestamp(CorbaAlgs::unpack_time(co_info.timestamp));
      Table::Row row(table->columns());

      row.add_field(co_info.option_id);
      row.add_field(co_info.token);
      row.add_field(co_info.type);
      row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));
      row.add_field(sequence_to_string(co_info.token_relations));

      table->add_row(row, filters_, sorter_);
    }
  }
  else if (aspect == command_adv_action)
  {
    for(unsigned long i = 0; i < config->adv_actions.length(); i++)
    {
      const AdServer::CampaignSvcs::AdvActionInfo& ai_info =
        config->adv_actions[i];

      Generics::Time timestamp(CorbaAlgs::unpack_time(ai_info.timestamp));
      Table::Row row(table->columns());

      row.add_field(ai_info.action_id);
      row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));

      std::set<unsigned long> ccg_ids;
      AdServer::CampaignSvcs::RevenueDecimal cur_value =
        AdServer::CampaignSvcs::RevenueDecimal::ZERO;

      CORBA::ULong j = 0;
      for(; j < ai_info.ccg_ids.length() && ai_info.ccg_ids[j] != 0; ++j)
      {
        ccg_ids.insert(ai_info.ccg_ids[j]);
      }

      if(j < ai_info.ccg_ids.length())
      {
        CorbaAlgs::unpack_decimal_from_seq(
          cur_value, ai_info.ccg_ids, j + 1);
      }

      row.add_field(cur_value.str());

      {
        std::ostringstream ostr;
        Algs::print(ostr, ccg_ids.begin(), ccg_ids.end());
        row.add_field(ostr.str());
      }

      table->add_row(row, filters_, sorter_);
    }
  }
  else if (aspect == command_category_channel)
  {
    for(unsigned long i = 0; i < config->category_channels.length(); i++)
    {
      const AdServer::CampaignSvcs::CategoryChannelInfo& channel_info =
        config->category_channels[i];

      Table::Row row(table->columns());
      row.add_field(channel_info.channel_id);
      row.add_field(channel_info.name.in());
      row.add_field(channel_info.newsgate_name.in());
      row.add_field(channel_info.parent_channel_id);
      row.add_field(channel_info.flags);
      row.add_field(
        CorbaAlgs::unpack_time(
          channel_info.timestamp).get_gm_time().format(TIME_FORMAT));
      std::ostringstream loc_ostr;
      for(CORBA::ULong i = 0; i < channel_info.localizations.length(); ++i)
      {
        loc_ostr << "(" << channel_info.localizations[i].language << ", '"
                 << channel_info.localizations[i].name << "') ";
      }
      row.add_field(loc_ostr.str());

      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_campaign || aspect == command_creative)
  {
    //Creating prefix for all subtables in current table;
    std::string prefix;
    AdminTools::create_prefix(
      prefix,
      columns_ptr,
      table->columns());

    AdServer::CampaignSvcs::ExpressionChannelHolderMap channel_holders;
    if(aspect == command_campaign)
    {
      fill_channels_(channel_holders, config->expression_channels);
    }

    for(unsigned long i = 0; i < config->campaigns.length(); i++)
    {
      const AdServer::CampaignSvcs::CampaignManager::AdaptedCampaignInfo&
        adapted_campaign_info = config->campaigns[i];
      const AdServer::CampaignSvcs::CampaignInfo& campaign_info =
        adapted_campaign_info.info;

      if(aspect == command_campaign)
      {
        Table::Row row(table->columns());
        describe_campaign(config,
          row, adapted_campaign_info, channel_holders, expand);
        table->add_row(row, filters_, sorter_);
      }
      else if(aspect == command_creative)
      {
        SizeMap sizes;
        fill_sizes_(sizes, config);

        for(unsigned long i = 0; i < campaign_info.creatives.length(); i++)
        {
          const AdServer::CampaignSvcs::CreativeInfo& creative =
            campaign_info.creatives[i];

          Table::Row row(table->columns());
          describe_creative(
            config,
            sizes,
            row,
            adapted_campaign_info,
            creative,
            expand,
            expand_options,
            prefix.c_str());

          table->add_row(row, filters_, sorter_);
        }
      }
    }
  }
  else if(aspect == command_geo_channel)
  {
    for(unsigned long i = 0; i < config->geo_channels.length(); i++)
    {
      Table::Row row(table->columns());
      describe_geo_channel(row, config->geo_channels[i]);
      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_geo_coord_channel)
  {
    for(unsigned long i = 0; i < config->geo_coord_channels.length(); i++)
    {
      Table::Row row(table->columns());
      describe_geo_coord_channel(row, config->geo_coord_channels[i]);
      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_web_operations)
  {
    for(unsigned long i = 0; i < config->web_operations.length(); i++)
    {
      Table::Row row(table->columns());
      row.add_field(config->web_operations[i].id);
      row.add_field(config->web_operations[i].app);
      row.add_field(config->web_operations[i].source);
      row.add_field(config->web_operations[i].operation);
      row.add_field(config->web_operations[i].flags);
      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_app_format)
  {
    for(CORBA::ULong i = 0; i < config->app_formats.length(); ++i)
    {
      Table::Row row(table->columns());
      row.add_field(config->app_formats[i].app_format);
      row.add_field(config->app_formats[i].mime_format);
      row.add_field(CorbaAlgs::unpack_time(
        config->app_formats[i].timestamp).get_gm_time().format(TIME_FORMAT));
      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_size)
  {
    for(CORBA::ULong i = 0; i < config->sizes.length(); ++i)
    {
      Table::Row row(table->columns());
      row.add_field(config->sizes[i].size_id);
      row.add_field(config->sizes[i].size_type_id);
      row.add_field(config->sizes[i].protocol_name);
      row.add_field(config->sizes[i].width);
      row.add_field(config->sizes[i].height);
      row.add_field(CorbaAlgs::unpack_time(
        config->sizes[i].timestamp).get_gm_time().format(TIME_FORMAT));
      table->add_row(row, filters_, sorter_);
    }
  }
  else if(aspect == command_contract)
  {
    for(CORBA::ULong i = 0; i < config->contracts.length(); ++i)
    {
      Table::Row row(table->columns());
      const auto& contract = config->contracts[i];
      row.add_field(contract.number);
      row.add_field(contract.date);
      row.add_field(contract.type);
      row.add_field(contract.vat_included);
      row.add_field(contract.ord_contract_id);
      row.add_field(contract.ord_ado_id);
      row.add_field(contract.subject_type);
      row.add_field(contract.action_type);
      row.add_field(contract.agent_acting_for_publisher);
      row.add_field(contract.parent_contract_id);
      row.add_field(contract.client_id);
      row.add_field(contract.client_name);
      row.add_field(contract.client_legal_form);
      row.add_field(contract.contractor_id);
      row.add_field(contract.contractor_name);
      row.add_field(contract.contractor_legal_form);
      row.add_field(CorbaAlgs::unpack_time(
        config->sizes[i].timestamp).get_gm_time().format(TIME_FORMAT));
      table->add_row(row, filters_, sorter_);
    }
  }

  table->dump(std::cout);

  return 0;
}

void Application::colocation_flags()
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  static const char* FUN = "Application::colocation_flags()";
  init_manager_ref_();
  try
  {
    AdServer::CampaignSvcs::ColocationFlagsSeq_var colocations =
      manager_->get_colocation_flags();
    print_colocation_flags_(colocations);
  }
  catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::ImplementationException: " <<
      ex.description << std::endl;
  }
  catch (const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

AdServer::CampaignSvcs::FreqCapInfo
Application::get_freq_cap(
  const AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
  unsigned long fc_id)
  /*throw(eh::Exception, CORBA::Exception)*/
{
  for(unsigned long i = 0; i < config->frequency_caps.length(); i++)
  {
    const AdServer::CampaignSvcs::FreqCapInfo& freq_cap_info =
      config->frequency_caps[i];

    if(freq_cap_info.fc_id == fc_id)
    {
      return freq_cap_info;
    }
  }

  AdServer::CampaignSvcs::FreqCapInfo result;

  result.fc_id = 0;
  result.lifelimit = 0;
  result.period = 0;
  result.window_limit = 0;
  result.window_time = 0;

  return result;
}

const AdServer::CampaignSvcs::AccountInfo*
Application::get_account(
  const AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
  unsigned long account_id)
  /*throw(eh::Exception, CORBA::Exception)*/
{
  for(unsigned long i = 0; i < config->accounts.length(); i++)
  {
    const AdServer::CampaignSvcs::AccountInfo& acc_info =
      config->accounts[i];

    if(acc_info.account_id == account_id)
    {
      return &acc_info;
    }
  }

  return 0;
}

int
Application::preview()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  init_manager_ref_();
  AdServer::CampaignSvcs::CampaignManager::CreativeParams params;

  params.format << *string_options[OPT_FORMAT].value;
  params.ccid = *ulong_options[OPT_ID].value;
  params.original_url << *string_options[OPT_REFERER].value;
  params.peer_ip << *string_options[OPT_IP].value;

  CORBA::String_var creative_body;
  if(manager_->get_campaign_creative_by_ccid(params, creative_body))
  {
    std::cout << "ccid : " << params.ccid
              << "\ncreative :\n" << creative_body.in() << std::endl;
  }
  else
  {
    std::cout << "ccid : 0\ncreative :\n";
  }

  return 0;
}

int
Application::trace_campaign_weight()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  init_manager_ref_();
  AdServer::CampaignSvcs::CampaignManager::RequestParams params;
  AdServer::CampaignSvcs::CampaignManager::AdSlotInfo ad_slot;

  params.common_info.request_id = CorbaAlgs::pack_request_id(
    AdServer::Commons::RequestId::create_random_based());
  params.common_info.colo_id = -1;
  params.common_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
  params.common_info.test_request = false;
  params.common_info.log_as_test = true;
  params.common_info.random = 0;
  params.common_info.original_url << *string_options[OPT_REFERER].value;
  params.common_info.time = CorbaAlgs::pack_time(Generics::Time::get_time_of_day());

  params.context_info.page_load_id = 0;
  params.profiling_available = true;
  params.fraud = false;
  params.disable_fraud_detection = true;
  params.tag_delivery_factor = 0;
  ad_slot.tag_id = 0;
  ad_slot.format << *string_options[OPT_FORMAT].value;
  ad_slot.passback = false;
  ad_slot.up_expand_space = 0xFFFFFFFF;
  ad_slot.right_expand_space = 0xFFFFFFFF;
  ad_slot.down_expand_space = 0xFFFFFFFF;
  ad_slot.left_expand_space = 0xFFFFFFFF;

  long campaign_id = -1;
  std::unique_ptr<AdServer::XslTransformer> xsl_transformer;

  if(ulong_options[OPT_ID].value.installed())
  {
    campaign_id = *ulong_options[OPT_ID].value;
  }
  else
  {
    throw Exception("Non defined campaign_id parameter.");
  }

  if(string_options[OPT_TID].value.installed())
  {
    unsigned long tag_id;
    Stream::Parser istr(*string_options[OPT_TID].value);
    istr >> tag_id;
    ad_slot.tag_id = tag_id;
  }

  if(string_options[OPT_CHANNELS].value.installed())
  {
    AdServer::CampaignSvcs::ChannelIdSeq& channels = params.channels;

    channels.length(0);

    String::StringManip::SplitComma
      tokenizer(*string_options[OPT_CHANNELS].value);

    String::SubString token;
    while (tokenizer.get_token(token))
    {
      unsigned long i = channels.length();
      channels.length(i + 1);
      Stream::Parser istr(token);
      istr >> channels[i];
    }
  }

  if(string_options[OPT_XSL].value.installed())
  {
    xsl_transformer = std::unique_ptr<AdServer::XslTransformer>(
      new AdServer::XslTransformer(string_options[OPT_XSL].value->c_str()));
  }
  if(ulong_options[OPT_COLO].value.installed())
  {
    params.common_info.colo_id = *ulong_options[OPT_COLO].value;
  }
  if(*ulong_options[OPT_OPTOUT].value)
  {
    params.common_info.user_status = AdServer::CampaignSvcs::US_OPTOUT;
  }
  if(string_options[OPT_COUNTRY].value.installed())
  {
    params.common_info.location.length(1);
    params.common_info.location[0].country << *string_options[OPT_COUNTRY].value;
  }
  if(ulong_options[OPT_TEST_REQ].value.installed())
  {
    params.common_info.test_request = *ulong_options[OPT_TEST_REQ].value;
  }

  {
    CORBA::String_var trace_xml;
    manager_->trace_campaign_selection(
      campaign_id,
      params,
      ad_slot,
      0, // cork for the auction_type parameter
      params.common_info.test_request,
      trace_xml);

    std::ostringstream xml_process_result;
    Stream::Parser xml_to_process(String::SubString(trace_xml.in()));

    if(xsl_transformer.get())
    {
      xsl_transformer->transform(xml_to_process, xml_process_result);
    }
    else
    {
      xml_process_result << xml_to_process.str();
    }

    std::cout << xml_process_result.str() << std::endl;
  }

  return 0;
}

int
Application::trace_campaign_index()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  init_manager_ref_();
  {
    CORBA::String_var trace_xml;
    manager_->trace_campaign_selection_index(trace_xml);
    std::cout << trace_xml.in() << std::endl;
  }

  return 0;
}

int
Application::select()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  init_manager_ref_();
  Generics::Time now = Generics::Time::get_time_of_day();

  AdServer::CampaignSvcs::CampaignManager::RequestParams request_params;
//  request_params.ccid = 0;
  request_params.common_info.user_status = AdServer::CampaignSvcs::US_OPTIN;

  AdServer::Commons::UserId user_id = AdServer::Commons::UserId::create_random_based();
  request_params.common_info.user_id = CorbaAlgs::pack_user_id(user_id);
  request_params.common_info.colo_id = -1;
  request_params.common_info.referer << *string_options[OPT_REFERER].value;
  request_params.common_info.peer_ip << *string_options[OPT_IP].value;
  request_params.common_info.colo_id = *ulong_options[OPT_COLO].value;
  request_params.common_info.creative_instantiate_type = "unsecure";
  if(string_options[OPT_COUNTRY].value.installed())
  {
    request_params.common_info.location.length(1);
    request_params.common_info.location[0].country << *string_options[OPT_COUNTRY].value;
  }
  if(string_options[OPT_UID].value.installed())
  {
    request_params.common_info.user_id = CorbaAlgs::pack_user_id(
      AdServer::Commons::UserId(*string_options[OPT_UID].value));
  }
  if(string_options[OPT_RANDOM].value.installed())
  {
    Stream::Parser istr(*string_options[OPT_RANDOM].value);
    istr >> request_params.common_info.random;
  }
  if(*ulong_options[OPT_OPTOUT].value)
  {
    request_params.common_info.user_status = AdServer::CampaignSvcs::US_OPTOUT;
  }

  request_params.common_info.time = CorbaAlgs::pack_time(now);
  request_params.context_info.client << *string_options[OPT_APP].value;
  request_params.context_info.client_version << *string_options[OPT_CLIENT_VERSION].value;
  request_params.context_info.platform << *string_options[OPT_OS].value;
  request_params.context_info.web_browser << *string_options[OPT_BROWSER].value;

  request_params.ad_slots.length(1);
  AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot =
    request_params.ad_slots[0];

  ad_slot.format << *string_options[OPT_FORMAT].value;

  if(string_options[OPT_TID].value.installed())
  {
    unsigned long tag_id;
    Stream::Parser istr(*string_options[OPT_TID].value);
    istr >> tag_id;
    ad_slot.tag_id = tag_id;
  }

  if(string_options[OPT_CHANNELS].value.installed())
  {
    AdServer::CampaignSvcs::ChannelIdSeq& channels =
      request_params.channels;

    channels.length(0);

    String::StringManip::SplitComma
      tokenizer(*string_options[OPT_CHANNELS].value);

    String::SubString token;
    while (tokenizer.get_token(token))
    {
      unsigned long i = channels.length();
      channels.length(i + 1);
      Stream::Parser istr(token);
      istr >> channels[i];
    }
  }

  AdServer::CampaignSvcs::CampaignManager::
    RequestCreativeResult_var request_result;

  request_params.need_debug_info = true;
  request_params.session_start = CorbaAlgs::pack_time(now);

  CORBA::String_var hostname;
  manager_->get_campaign_creative(
    request_params,
    hostname,
    request_result);

  const AdServer::CampaignSvcs::CampaignManager::AdSlotResult&
    ad_slot_result = request_result->ad_slots[0];
  const AdServer::CampaignSvcs::CampaignManager::CreativeSelectResultSeq&
    selected_creatives = ad_slot_result.selected_creatives;
  const AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo&
    ad_slot_debug_info = ad_slot_result.debug_info;

  for(CORBA::ULong i = 0; i < selected_creatives.length(); ++i)
  {
    const AdServer::CampaignSvcs::CampaignManager::CreativeSelectResult&
      creative_select_result = selected_creatives[i];
    const AdServer::CampaignSvcs::CampaignManager::CreativeSelectDebugInfo&
      creative_select_debug_info = ad_slot_debug_info.selected_creatives[i];

    std::cout << "\nccid : " << creative_select_result.ccid <<
      "\ncreative_size : " <<
      creative_select_result.creative_size.in() <<
      "\ncampaign_id : " << creative_select_result.cmp_id <<
      "\ntriggered_expression : " <<
      creative_select_debug_info.triggered_expression.in() <<
      "\nclick_url : " <<
      creative_select_result.click_url.in();
  }

  std::cout << "\ncreative :\n" << ad_slot_result.creative_body.in() << std::endl;

  return 0;
}

int Application::decode()
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{

  if(!string_options[OPT_TRIAL_OPT_IN].value.installed())
  {
    Stream::Error ostr;
    ostr << "didn't trialoptin option for decode";
    throw InvalidArgument(ostr);
  }

  const std::string& trial_opt_in = *string_options[OPT_TRIAL_OPT_IN].value;

  Generics::Time trial_opt_in_time;

  AdServer::OptInDays::load_opt_in_days(trial_opt_in,
    trial_opt_in_time);
  std::cout << "trialoptin: " <<
    trial_opt_in_time.get_gm_time().format(TIME_FORMAT) << std::endl;

  return 0;
}

void Application::fill_channels_(
  AdServer::CampaignSvcs::ExpressionChannelHolderMap& channel_holders,
  const AdServer::CampaignSvcs::ExpressionChannelSeq& channels)
  noexcept
{
  for(CORBA::ULong i = 0; i < channels.length(); ++i)
  {
    AdServer::CampaignSvcs::ExpressionChannelBase_var channel =
      AdServer::CampaignSvcs::unpack_channel(
        channels[i], channel_holders);

    AdServer::CampaignSvcs::ExpressionChannelHolderMap::iterator ch_it =
      channel_holders.find(channels[i].channel_id);

    if(ch_it == channel_holders.end())
    {
      channel_holders.insert(std::make_pair(
        channels[i].channel_id,
        AdServer::CampaignSvcs::ExpressionChannelHolder_var(
          new AdServer::CampaignSvcs::ExpressionChannelHolder(channel))));
    }
    else
    {
      ch_it->second->channel = channel;
    }
  }
}

std::string
Application::creatives(const AdServer::CampaignSvcs::CreativeSeq& creatives)
  /*throw(eh::Exception)*/
{
  std::ostringstream ostr;
  for(unsigned long i = 0; i < creatives.length(); i++)
  {
    const AdServer::CampaignSvcs::CreativeInfo& creative = creatives[i];
    ostr << (i > 0 ? ", " : "") << creative.ccid;
  }

  return ostr.str();
}

std::string
Application::campaign_sites(
  const AdServer::CampaignSvcs::CampaignInfo& campaign,
  const AdServer::CampaignSvcs::CampaignManager::CampaignConfig* /*config*/)
  /*throw(eh::Exception)*/
{
  std::ostringstream sites_ostr;
  CorbaAlgs::print_sequence(sites_ostr, campaign.sites);
  return sites_ostr.str();
}

std::string Application::campaign_weekly_run(
  const AdServer::CampaignSvcs::CampaignInfo& campaign)
  /*throw(eh::Exception)*/
{
  if(campaign.weekly_run_intervals.length() == 0)
  {
    return "none";
  }

  std::ostringstream ostr;

  for(unsigned long i = 0; i < campaign.weekly_run_intervals.length(); ++i)
  {
    ostr << (i != 0 ? ",(" : "(") <<
      weekly_run_convert(campaign.weekly_run_intervals[i].min) <<
      "," << weekly_run_convert(campaign.weekly_run_intervals[i].max) << ")";
  }

  return ostr.str();
}

std::string
Application::decode_campaign_flags(
  unsigned long flags) noexcept
{
  using namespace AdServer::CampaignSvcs;

  std::ostringstream ostr;
  ostr << flags << " ";
  if(flags & CampaignFlags::INCLUDE_SPECIFIC_SITES)
  {
    ostr << "(specific sites)";
  }
  if(flags & CampaignFlags::TRACK_ACTIONS)
  {
    ostr << "(track actions)";
  }
  if(flags & CampaignFlags::US_NONE)
  {
    ostr << "(us: none)";
  }
  if(flags & CampaignFlags::US_OPTIN)
  {
    ostr << "(us: optin)";
  }
  if(flags & CampaignFlags::US_OPTOUT)
  {
    ostr << "(us: optout)";
  }
  if(flags & CampaignFlags::US_UNDEFINED)
  {
    ostr << "(us: undefined)";
  }
  return ostr.str();
}

std::string
Application::decode_bid_strategy(
  unsigned long bid_strategy)
  noexcept
{
  if(bid_strategy == AdServer::CampaignSvcs::BS_MAX_REACH)
  {
    return "max-reach";
  }
  else if(bid_strategy == AdServer::CampaignSvcs::BS_MIN_CTR_GOAL)
  {
    return "min-ctr-goal";
  }

  return "unknown";
}

std::string Application::decode_account_type_flags(
  unsigned long flags) noexcept
{
  using namespace AdServer::CampaignSvcs;

  std::ostringstream ostr;
  ostr << flags << " ";
  if(flags & AccountTypeFlags::GROSS)
  {
    ostr << "(gross)";
  }
  else
  {
    ostr << "(net)";
  }

  if(flags & AccountTypeFlags::INVOICE_COMMISION)
  {
    ostr << "(invoice commision)";
  }

  if(flags & AccountTypeFlags::USE_SELF_BUDGET)
  {
    ostr << "(advertiser budget)";
  }
  else
  {
    ostr << "(agency budget)";
  }

  if(flags & AccountTypeFlags::AGENCY_PROFIT_BY_PUB_AMOUNT)
  {
    ostr << "(agency profit by pub amount)";
  }
  else
  {
    ostr << "(fix price)";
  }

  return ostr.str();
}

std::string
decode_ad_request_type(unsigned long ad_request_type_val)
{
  AdServer::CampaignSvcs::AdRequestType ad_request_type =
    static_cast<AdServer::CampaignSvcs::AdRequestType>(ad_request_type_val);

  if(ad_request_type == AdServer::CampaignSvcs::AR_NORMAL)
  {
    return "normal";
  }

  if(ad_request_type == AdServer::CampaignSvcs::AR_OPENRTB)
  {
    return "openrtb";
  }

  if(ad_request_type == AdServer::CampaignSvcs::AR_OPENRTB_WITH_CLICKURL)
  {
    return "openrtb-with-click";
  }

  if(ad_request_type == AdServer::CampaignSvcs::AR_OPENX)
  {
    return "openx";
  }

  if(ad_request_type == AdServer::CampaignSvcs::AR_APPNEXUS)
  {
    return "appnexus";
  }

  if(ad_request_type == AdServer::CampaignSvcs::AR_GOOGLE)
  {
    return "google";
  }

  return "unknown";
}

void Application::describe_colocation(
  AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
  Table::Row& row,
  const AdServer::CampaignSvcs::ColocationInfo& colocation_info,
  const char* prefix,
  bool expand_options)
{
  Generics::Time timestamp(CorbaAlgs::unpack_time(colocation_info.timestamp));

  row.add_field(colocation_info.colo_name);
  row.add_field(colocation_info.colo_id);
  row.add_field(colocation_info.colo_rate_id);
  row.add_field(colocation_info.account_id);
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(colocation_info.revenue_share).str());

  std::string ad_serving_str;
  AdServer::CampaignSvcs::ColocationAdServingType ad_serving =
    static_cast<AdServer::CampaignSvcs::ColocationAdServingType>(
      colocation_info.ad_serving);
  if(ad_serving == AdServer::CampaignSvcs::CS_ALL)
  {
    ad_serving_str = "all";
  }
  else if(ad_serving == AdServer::CampaignSvcs::CS_ONLY_OPTIN)
  {
    ad_serving_str = "only optin";
  }
  else if(ad_serving == AdServer::CampaignSvcs::CS_NON_OPTOUT)
  {
    ad_serving_str = "non optout";
  }
  else if(ad_serving == AdServer::CampaignSvcs::CS_NONE)
  {
    ad_serving_str = "none";
  }
  else
  {
    ad_serving_str = "unknown";
  }

  row.add_field(ad_serving_str);
  row.add_field(colocation_info.hid_profile);

  std::string options_str;
  print_custom_options(config, options_str, colocation_info.tokens, prefix, expand_options);
  row.add_field(options_str);

  row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));
}

void Application::describe_country(
  AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
  Table::Row& row,
  const AdServer::CampaignSvcs::CountryInfo& country_info,
  const char* prefix,
  bool expand_options)
{
  Generics::Time timestamp(CorbaAlgs::unpack_time(country_info.timestamp));

  row.add_field(country_info.country_code);

  std::string options_str;
  print_custom_options(config, options_str, country_info.tokens, prefix, expand_options);
  row.add_field(options_str);

  row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));
}

void Application::describe_geo_channel(
  Table::Row& row,
  const AdServer::CampaignSvcs::GeoChannelInfo& geo_channel_info)
{
  assert(geo_channel_info.geoip_targets.length() == 1);

  row.add_field(geo_channel_info.channel_id);
  row.add_field(geo_channel_info.country);
  row.add_field(geo_channel_info.geoip_targets[0].region);
  row.add_field(geo_channel_info.geoip_targets[0].city);
}

void Application::describe_geo_coord_channel(
  Table::Row& row,
  const AdServer::CampaignSvcs::GeoCoordChannelInfo& geo_coord_channel_info)
{
  row.add_field(geo_coord_channel_info.channel_id);
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::CoordDecimal>(geo_coord_channel_info.longitude));
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::CoordDecimal>(geo_coord_channel_info.latitude));
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::AccuracyDecimal>(geo_coord_channel_info.radius));
}

void Application::describe_site(
  AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
  Table::Row& row,
  const AdServer::CampaignSvcs::SiteInfo& site_info,
  bool expand)
{
  Generics::Time timestamp(CorbaAlgs::unpack_time(site_info.timestamp));

  row.add_field(site_info.site_id);

  row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));

  row.add_field(site_info.status);
  row.add_field(site_info.noads_timeout);

  row.add_field(site_info.freq_cap_id);

  if(expand)
  {
    AdServer::CampaignSvcs::FreqCapInfo freq_cap =
    get_freq_cap(config, site_info.freq_cap_id);

    row.add_field(freq_cap.lifelimit);
    row.add_field(freq_cap.period);
    row.add_field(freq_cap.window_limit);
    row.add_field(freq_cap.window_time);
  }

  row.add_field(sequence_to_string(site_info.approved_creative_categories));
  row.add_field(sequence_to_string(site_info.rejected_creative_categories));
  row.add_field(sequence_to_string(site_info.approved_creatives));
  row.add_field(sequence_to_string(site_info.rejected_creatives));

  row.add_field(site_info.flags);
  row.add_field(site_info.account_id);
}

void
Application::describe_tag(
  AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
  const SizeMap& sizes,
  Table::Row& row,
  const AdServer::CampaignSvcs::CampaignManager::AdaptedTagInfo&
    adapted_tag_info,
  const char* prefix,
  bool expand_options)
{
  const AdServer::CampaignSvcs::TagInfo& tag_info = adapted_tag_info.info;

  Generics::Time timestamp(CorbaAlgs::unpack_time(tag_info.timestamp));

  row.add_field(tag_info.tag_id);
  row.add_field(tag_info.site_id);
  row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));

  {
    std::ostringstream ostr;
    for(CORBA::ULong size_i = 0; size_i < tag_info.sizes.length(); ++size_i)
    {
      ostr << "[" << tag_info.sizes[size_i].size_id << ",'";
      SizeMap::const_iterator size_it = sizes.find(tag_info.sizes[size_i].size_id);
      if(size_it != sizes.end())
      {
        ostr << size_it->second->protocol_name;
      }
      ostr << "'," <<
        tag_info.sizes[size_i].max_text_creatives << "]";
    }
    row.add_field(ostr.str());
  }

  row.add_field(tag_info.imp_track_pixel);
  row.add_field(tag_info.passback);
  row.add_field(tag_info.passback_type);
  row.add_field(tag_info.flags);
  row.add_field(tag_info.marketplace);
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(tag_info.adjustment).str());

  row.add_field(sequence_to_string(tag_info.accepted_categories));
  row.add_field(sequence_to_string(tag_info.rejected_categories));
  row.add_field(tag_info.allow_expandable);
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(tag_info.auction_max_ecpm_share).str());
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(tag_info.auction_prop_probability_share).str());
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(tag_info.auction_random_share).str());
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(tag_info.cost_coef).str());
  row.add_field(CorbaAlgs::unpack_time(
    tag_info.tag_pricings_timestamp).get_gm_time().format(TIME_FORMAT));

  std::ostringstream tag_pricings_str;
  tag_pricings_str << "[country ccg_type ccg_rate_type site_rate_id cpm revenue share]";
  int field_len = 9;

  for(unsigned int j = 0; j < tag_info.tag_pricings.length(); ++j)
  {
    const AdServer::CampaignSvcs::TagPricingInfo&
      tag_pricing = tag_info.tag_pricings[j];

    tag_pricings_str << std::endl;
    tag_pricings_str.width(30);
    tag_pricings_str << " ";
    tag_pricings_str.width(field_len);
    tag_pricings_str << "'" << tag_pricing.country_code << "'";
    tag_pricings_str.width(field_len);
    tag_pricings_str << ccg_type_to_string(
      static_cast<AdServer::CampaignSvcs::CCGType>(tag_pricing.ccg_type));
    tag_pricings_str.width(field_len);
    tag_pricings_str << ccg_rate_type_to_string(
      static_cast<AdServer::CampaignSvcs::CCGRateType>(tag_pricing.ccg_rate_type));
    tag_pricings_str.width(field_len);
    tag_pricings_str << tag_pricing.site_rate_id;
    tag_pricings_str.width(field_len);
    tag_pricings_str << CorbaAlgs::unpack_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(
        adapted_tag_info.cpms[j]).integer<uint64_t>();
    tag_pricings_str.width(field_len);
    tag_pricings_str << CorbaAlgs::unpack_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(
        tag_pricing.imp_revenue).str();
    tag_pricings_str.width(field_len);
    tag_pricings_str << CorbaAlgs::unpack_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(
        tag_pricing.revenue_share).str();
  }
  row.add_field(tag_pricings_str.str());

  {
    std::string options_str;
    print_custom_options(config,
      options_str, tag_info.hidden_tokens, prefix, expand_options);
    row.add_field(options_str);
  }

  {
    std::string options_str;
    print_custom_options(config,
      options_str, tag_info.passback_tokens, prefix, expand_options);
    row.add_field(options_str);
  }

  std::ostringstream options_ostr;

  {
    std::string options_str;
    print_custom_options(config,
      options_str, tag_info.tokens, prefix, expand_options);
    options_ostr << options_str;
  }

  for(CORBA::ULong templ_tokens_i = 0;
      templ_tokens_i < tag_info.template_tokens.length();
      ++templ_tokens_i)
  {
    std::string options_str;
    print_custom_options(
      config,
      options_str,
      tag_info.template_tokens[templ_tokens_i].tokens,
      (std::string(prefix) + "  ").c_str(),
      expand_options);
    options_ostr << std::endl <<
      prefix << "<" << tag_info.template_tokens[templ_tokens_i].template_name <<
      "(template)>" << options_str;
  }

  for(CORBA::ULong size_tokens_i = 0;
      size_tokens_i < tag_info.sizes.length();
      ++size_tokens_i)
  {
    std::string hidden_options_str;
    print_custom_options(
      config,
      hidden_options_str,
      tag_info.sizes[size_tokens_i].hidden_tokens,
      (std::string(prefix) + "  ").c_str(),
      expand_options);
    options_ostr << std::endl <<
      prefix << "<" << tag_info.sizes[size_tokens_i].size_id <<
      "(size)>" << hidden_options_str;

    std::string options_str;
    print_custom_options(
      config,
      options_str,
      tag_info.sizes[size_tokens_i].tokens,
      (std::string(prefix) + "  ").c_str(),
      expand_options);
    options_ostr << std::endl <<
      prefix << "<" << tag_info.sizes[size_tokens_i].size_id <<
      "(size)>" << options_str;
  }

  row.add_field(options_ostr.str());
}

void Application::describe_account(
  AdServer::CampaignSvcs::CampaignManager::CampaignConfig* /*config*/,
  Table::Row& row,
  const AdServer::CampaignSvcs::AccountInfo& acc_info)
{
  Generics::Time timestamp(CorbaAlgs::unpack_time(acc_info.timestamp));

  row.add_field(acc_info.account_id);
  row.add_field(acc_info.agency_account_id);
  row.add_field(acc_info.internal_account_id);
  row.add_field(acc_info.status);
  row.add_field(acc_info.eval_status);
  row.add_field(acc_info.legal_name);
  row.add_field(acc_info.flags);
  row.add_field(decode_account_type_flags(acc_info.at_flags));
  row.add_field(acc_info.text_adserving);
  row.add_field(acc_info.currency_id);
  row.add_field(acc_info.country);
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(acc_info.commision).str());
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(acc_info.budget).str());
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(acc_info.paid_amount).str());

  {
    Generics::Time time_offset = CorbaAlgs::unpack_time(acc_info.time_offset);
    std::ostringstream ostr;
    const Generics::Time::Print& print = time_offset.print();
    ostr << (print.sign >= 0 ? '+' : '-') <<
      Generics::Time(print.integer_part).get_gm_time().format("%H:%M");
    row.add_field(ostr.str());
  }

  row.add_field(sequence_to_string(acc_info.walled_garden_accounts));
  AdServer::CampaignSvcs::AuctionRateType auction_rate =
    static_cast<AdServer::CampaignSvcs::AuctionRateType>(acc_info.auction_rate);
  row.add_field(auction_rate == AdServer::CampaignSvcs::AR_GROSS ? "gross" : "net");
  row.add_field(acc_info.use_pub_pixels);
  row.add_field(acc_info.pub_pixel_optin);
  row.add_field(acc_info.pub_pixel_optout);
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(acc_info.self_service_commission).str());
  row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));
}

void Application::describe_creative_category(
  AdServer::CampaignSvcs::CampaignManager::CampaignConfig* /*config*/,
  Table::Row& row,
  const AdServer::CampaignSvcs::CreativeCategoryInfo& creative_category_info)
{
  row.add_field(creative_category_info.creative_category_id);
  row.add_field(creative_category_info.cct_id);
  row.add_field(creative_category_info.name);

  std::ostringstream ec_ostr;
  for(CORBA::ULong ec_i = 0;
    ec_i < creative_category_info.external_categories.length(); ++ec_i)
  {
    ec_ostr << "[ " << decode_ad_request_type(
      creative_category_info.external_categories[ec_i].ad_request_type) << ": ";
    CorbaAlgs::print_sequence(
      ec_ostr,
      creative_category_info.external_categories[ec_i].names);
    ec_ostr << " ]";
  }

  row.add_field(ec_ostr.str());
  row.add_field(CorbaAlgs::unpack_time(
    creative_category_info.timestamp).get_gm_time().format(TIME_FORMAT));
}

void Application::describe_campaign(
  AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
  Table::Row& row,
  const AdServer::CampaignSvcs::CampaignManager::AdaptedCampaignInfo& campaign,
  AdServer::CampaignSvcs::ExpressionChannelHolderMap& channels,
  bool expand)
{
  static const char* FUN = "Application::describe_campaign()";

  const AdServer::CampaignSvcs::CampaignInfo& campaign_info = campaign.info;

  Generics::Time timestamp(
    CorbaAlgs::unpack_time(campaign_info.timestamp));

  row.add_field(campaign_info.campaign_id);
  row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));
  row.add_field(campaign_info.account_id);

  if(expand)
  {
    const AdServer::CampaignSvcs::AccountInfo* acc_info =
      get_account(config, campaign_info.account_id);

    if(acc_info)
    {
      Generics::Time timestamp(CorbaAlgs::unpack_time(acc_info->timestamp));

      row.add_field(acc_info->flags);
      row.add_field(acc_info->text_adserving);
      row.add_field(acc_info->currency_id);
      row.add_field(timestamp.get_gm_time().format(TIME_FORMAT));
    }
    else
    {
      Stream::Error ostr;
      ostr << FUN << "not found account: " << campaign_info.account_id;
      throw Exception(ostr);
    }
  }

  row.add_field(campaign_info.advertiser_id);

  row.add_field(campaign_info.campaign_group_id);

  row.add_field(campaign_info.ccg_rate_id);
  row.add_field(campaign_info.ccg_rate_type);
  row.add_field(campaign_info.fc_id);

  if(expand)
  {
    AdServer::CampaignSvcs::FreqCapInfo freq_cap =
      get_freq_cap(config, campaign_info.fc_id);

    row.add_field(freq_cap.lifelimit);
    row.add_field(freq_cap.period);
    row.add_field(freq_cap.window_limit);
    row.add_field(freq_cap.window_time);
  }

  row.add_field(campaign_info.group_fc_id);

  if(expand)
  {
    AdServer::CampaignSvcs::FreqCapInfo freq_cap =
      get_freq_cap(config, campaign_info.group_fc_id);

    row.add_field(freq_cap.lifelimit);
    row.add_field(freq_cap.period);
    row.add_field(freq_cap.window_limit);
    row.add_field(freq_cap.window_time);
  }

  row.add_field(campaign_weekly_run(campaign_info));
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(campaign.ecpm));
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(campaign.ctr).str());

  {
    std::ostringstream channel_str;
    AdServer::CampaignSvcs::ExpressionChannel::Expression expr;
    AdServer::CampaignSvcs::unpack_expression(
      expr, campaign_info.expression, channels);
    AdServer::CampaignSvcs::ExpressionChannel_var ch =
      new AdServer::CampaignSvcs::ExpressionChannel(expr);
    AdServer::CampaignSvcs::print(channel_str, ch);
    row.add_field(channel_str.str());
  }

  {
    std::ostringstream channel_str;
    AdServer::CampaignSvcs::ExpressionChannel::Expression expr;
    AdServer::CampaignSvcs::unpack_expression(
      expr, campaign_info.stat_expression, channels);
    AdServer::CampaignSvcs::ExpressionChannel_var ch =
      new AdServer::CampaignSvcs::ExpressionChannel(expr);
    AdServer::CampaignSvcs::print(channel_str, ch);
    row.add_field(channel_str.str());
  }

  row.add_field(campaign_info.country);
  row.add_field(campaign_sites(campaign_info, config));
  row.add_field(creatives(campaign_info.creatives));

  row.add_field(campaign_info.status);
  row.add_field(campaign_info.eval_status);
  row.add_field(campaign_info.ccg_type);
  row.add_field(campaign_info.target_type);
  row.add_field(campaign_info.start_user_group_id);
  row.add_field(campaign_info.end_user_group_id);
  row.add_field(campaign_info.ctr_reset_id);
  row.add_field(campaign_info.random_imps);
  row.add_field(campaign_mode_to_string(
    static_cast<AdServer::CampaignSvcs::CampaignMode>(campaign_info.mode)));
  row.add_field(CorbaAlgs::unpack_time(campaign_info.min_uid_age).gm_ft());
  row.add_field(campaign_info.seq_set_rotate_imps);

  row.add_field(CorbaAlgs::unpack_time(
    campaign_info.ccg_delivery_limits.date_start).get_gm_time().format(TIME_FORMAT));
  row.add_field(CorbaAlgs::unpack_time(
    campaign_info.ccg_delivery_limits.date_end).get_gm_time().format(TIME_FORMAT));
  row.add_field(optional_decimal_to_string(
    campaign_info.ccg_delivery_limits.budget));
  row.add_field(optional_decimal_to_string(
    campaign_info.ccg_delivery_limits.daily_budget));
  row.add_field(
    campaign_info.ccg_delivery_limits.delivery_pacing);
  row.add_field(
    campaign_info.ccg_delivery_limits.imps_defined ?
    int_to_str(campaign_info.ccg_delivery_limits.imps) : std::string());
  row.add_field(std::string());
  /*
  row.add_field(
    campaign_info.ccg_delivery_limits.daily_imps_defined ?
    int_to_str(campaign_info.ccg_delivery_limits.daily_imps) : std::string());
  */
  row.add_field(
    campaign_info.ccg_delivery_limits.clicks_defined ?
    int_to_str(campaign_info.ccg_delivery_limits.clicks) : std::string());
  row.add_field(std::string());
  /*
  row.add_field(
    campaign_info.ccg_delivery_limits.daily_clicks_defined ?
    int_to_str(campaign_info.ccg_delivery_limits.daily_clicks) : std::string());
  */

  row.add_field(CorbaAlgs::unpack_time(
    campaign_info.campaign_delivery_limits.date_start).get_gm_time().format(TIME_FORMAT));
  row.add_field(CorbaAlgs::unpack_time(
    campaign_info.campaign_delivery_limits.date_end).get_gm_time().format(TIME_FORMAT));
  row.add_field(optional_decimal_to_string(
    campaign_info.campaign_delivery_limits.budget));
  row.add_field(optional_decimal_to_string(
    campaign_info.campaign_delivery_limits.daily_budget));
  row.add_field(
    campaign_info.campaign_delivery_limits.delivery_pacing);
  row.add_field(
    campaign_info.campaign_delivery_limits.imps_defined ?
    int_to_str(campaign_info.campaign_delivery_limits.imps) : std::string());

  row.add_field(std::string());
  /*
  row.add_field(
    campaign_info.campaign_delivery_limits.daily_imps_defined ?
    int_to_str(campaign_info.campaign_delivery_limits.daily_imps) : std::string());
  */
  row.add_field(
    campaign_info.campaign_delivery_limits.clicks_defined ?
    int_to_str(campaign_info.campaign_delivery_limits.clicks) : std::string());
  row.add_field(std::string());
  /*
  row.add_field(
    campaign_info.campaign_delivery_limits.daily_clicks_defined ?
    int_to_str(campaign_info.campaign_delivery_limits.daily_clicks) : std::string());
  */

  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(campaign_info.max_pub_share).str());
  row.add_field(decode_bid_strategy(campaign_info.bid_strategy));
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(campaign_info.min_ctr_goal).str());
  row.add_field(CorbaAlgs::unpack_decimal<
    AdServer::CampaignSvcs::RevenueDecimal>(campaign_info.commision).str());
  row.add_field(decode_campaign_flags(campaign_info.flags));
  row.add_field(campaign_info.marketplace);

  if(expand)
  {
    row.add_field(sequence_to_string(campaign_info.colocations));
    row.add_field(sequence_to_string(campaign_info.exclude_pub_accounts));
    {
      std::ostringstream ostr;
      for(CORBA::ULong i = 0; i < campaign_info.exclude_tags.length(); ++i)
      {
        ostr << "[ " << campaign_info.exclude_tags[i].tag_id << ", " <<
          campaign_info.exclude_tags[i].delivery_value << "] ";
      }
      row.add_field(ostr.str());
    }
    row.add_field(campaign_info.delivery_coef);
    row.add_field(CorbaAlgs::unpack_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(campaign_info.imp_revenue).str());
    row.add_field(CorbaAlgs::unpack_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(campaign_info.click_revenue).str());
    row.add_field(CorbaAlgs::unpack_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(campaign_info.action_revenue).str());
  }
}

void Application::print_custom_options(
  AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
  std::string& str,
  const AdServer::CampaignSvcs::OptionValueSeq& tokens,
  const char* prefix,
  bool expand_options)
{
  if (!prefix)
  {
    prefix = "";
  }

  CORBA::ULong ind, columns = tokens.length();

  if(expand_options)
  {
    Table table_var(columns);

    //Adding columns;
    for(ind = 0; ind < columns; ind++)
    {
      std::ostringstream option_id_str;
      for (unsigned long i = 0; i < config->creative_options.length(); i++)
      {
        if (config->creative_options[i].option_id == tokens[ind].option_id)
        {
          option_id_str << config->creative_options[i].token;
          break;
        }
      }
      option_id_str << "(" << tokens[ind].option_id << ")";
      table_var.column(
        ind,
        Table::Column(option_id_str.str().c_str(), Table::Column::TEXT));
    }

    Table::Row row_var(columns);

    //Adding values;
    for (ind = 0; ind < columns; ++ind)
    {
      std::string field = tokens[ind].value.in();
      if (!expand_options)
      {
        size_t separator = field.find('\n');
        if (separator != std::string::npos &&
            field.find('\n', separator + 1) != std::string::npos)
        {
          field = field.substr(0, separator - 1) + "...";
        }
      }
      else
      {
        size_t separator = field.find('\n');
        if (separator != std::string::npos &&
            field.find('\n', separator + 1) != std::string::npos)
        {
          field = '\n' + field;
        }
      }
      row_var.add_field(field);
    }

    table_var.add_row(row_var);

    //Dumping subtable to stringstream;
    std::ostringstream ostr(std::ostringstream::out);
    ostr << std::endl;
    table_var.dump(ostr, prefix);

    //Skipping trailing '\n' and adding the result into parent table;
    size_t main_part = ostr.str().find_last_not_of("\n");
    if (main_part != std::string::npos)
    {
      str = ostr.str().substr(0, main_part + 1);
    }
  }
  else
  {
    std::ostringstream ostr;
    for(CORBA::ULong i = 0; i < tokens.length(); ++i)
    {
      if(i != 0)
      {
        ostr << ", ";
      }
      ostr << tokens[i].option_id;
    }
    str = ostr.str();
  }
}

void Application::describe_creative(
  AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
  const SizeMap& sizes,
  Table::Row& row,
  const AdServer::CampaignSvcs::CampaignManager::AdaptedCampaignInfo&
    adapted_campaign_info,
  const AdServer::CampaignSvcs::CreativeInfo& creative,
  bool expand,
  bool expand_options,
  const char* prefix)
{
  const AdServer::CampaignSvcs::CampaignInfo& campaign_info =
    adapted_campaign_info.info;

  row.add_field(creative.ccid);
  row.add_field(creative.creative_id);
  row.add_field(campaign_info.campaign_id);

  if(expand)
  {
    row.add_field(campaign_info.campaign_group_id);
  }

  row.add_field(creative.fc_id);

  if(expand)
  {
    AdServer::CampaignSvcs::FreqCapInfo freq_cap =
      get_freq_cap(config, creative.fc_id);

    row.add_field(freq_cap.lifelimit);
    row.add_field(freq_cap.period);
    row.add_field(freq_cap.window_limit);
    row.add_field(freq_cap.window_time);
  }

  row.add_field(creative.weight);
  row.add_field(creative.creative_format);
  row.add_field(creative.click_url.value);

  {
    std::ostringstream ostr;
    for(CORBA::ULong size_i = 0; size_i < creative.sizes.length(); ++size_i)
    {
      ostr << "[" << creative.sizes[size_i].size_id << ",'";
      SizeMap::const_iterator size_it = sizes.find(creative.sizes[size_i].size_id);
      if(size_it != sizes.end())
      {
        ostr << size_it->second->protocol_name;
      }
      ostr << "'," << creative.sizes[size_i].up_expand_space <<
        "," << creative.sizes[size_i].right_expand_space <<
        "," << creative.sizes[size_i].down_expand_space <<
        "," << creative.sizes[size_i].left_expand_space << "]";
    }
    row.add_field(ostr.str());
  }

  row.add_field(creative.version_id);
  row.add_field(creative.status);

  std::ostringstream ctg_ostr;
  CorbaAlgs::print_sequence(ctg_ostr, creative.categories);
  row.add_field(ctg_ostr.str());
  row.add_field(creative.order_set_id);
  row.add_field(creative.initial_contract_id);

  if(expand)
  {
    row.add_field(campaign_info.fc_id);

    {
      AdServer::CampaignSvcs::FreqCapInfo freq_cap =
        get_freq_cap(config, campaign_info.fc_id);

      row.add_field(freq_cap.lifelimit);
      row.add_field(freq_cap.period);
      row.add_field(freq_cap.window_limit);
      row.add_field(freq_cap.window_time);
    }

    row.add_field(campaign_info.group_fc_id);

    {
      AdServer::CampaignSvcs::FreqCapInfo freq_cap =
        get_freq_cap(config, campaign_info.group_fc_id);

      row.add_field(freq_cap.lifelimit);
      row.add_field(freq_cap.period);
      row.add_field(freq_cap.window_limit);
      row.add_field(freq_cap.window_time);
    }

    row.add_field(campaign_weekly_run(campaign_info));
    row.add_field(CorbaAlgs::unpack_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(adapted_campaign_info.ecpm));
    row.add_field(CorbaAlgs::unpack_decimal<
      AdServer::CampaignSvcs::RevenueDecimal>(adapted_campaign_info.ctr
        ).str());

    row.add_field(decode_campaign_flags(campaign_info.flags));
    row.add_field(campaign_info.country);
    row.add_field(campaign_sites(campaign_info, config));
    row.add_field(creatives(campaign_info.creatives));

    row.add_field(campaign_info.status);
  }

  std::ostringstream options_ostr;

  {
    std::string options_str;
    print_custom_options(config, options_str, creative.tokens, prefix, expand_options);
    options_ostr << options_str;
  }

  for(CORBA::ULong size_i = 0; size_i < creative.sizes.length(); ++size_i)
  {
    std::string size_options_str;
    print_custom_options(config,
      size_options_str, creative.sizes[size_i].tokens, prefix, expand_options);
    options_ostr << std::endl <<
      prefix << "<" << creative.sizes[size_i].size_id <<
      "(size)>" << size_options_str;
  }
  row.add_field(options_ostr.str());
}

void
Application::fill_sizes_(
  SizeMap& sizes,
  const AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config)
  noexcept
{
  for(CORBA::ULong size_i = 0; size_i < config->sizes.length(); ++size_i)
  {
    sizes.insert(std::make_pair(
      config->sizes[size_i].size_id,
      &config->sizes[size_i]));
  }
}
