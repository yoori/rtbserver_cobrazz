
#include <memory>
#include <iostream>
#include <sstream>
#include <string>

#include <String/StringManip.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <UtilCommons/Table.hpp>

#include "Application.hpp"

namespace {

const char USAGE[] =
  "\nUsage:\nLogGeneralizerAdmin <command> <command arguments>\n\n"
  "Synopsis 1:\n"
  "\tLogGeneralizerAdmin help\n\n"
  "Synopsis 2:\n"
  "\tLogGeneralizerAdmin stop-stat-upload "
  " -r <log generalizer object reference> --client-id=<NUMBER>\n\n"
  "\tLogGeneralizerAdmin start-stat-upload "
  " -r <log generalizer object reference> --client-id=<NUMBER>"
  " [--clear]\n\n"
  "\tLogGeneralizerAdmin get-stat-info "
  " -r <log generalizer object reference> --client-id=<NUMBER>"
  " [--clear]\n\n"
  "Sample:\n"
  "\tLogGeneralizerAdmin stop-stat-upload"
  " -r corbaloc:iiop:srv-dev.ocslab.com:18011/LogGeneralizer --client-id=1\n"
  "\tLogGeneralizerAdmin start-stat-upload"
  " -r corbaloc:iiop:srv-dev.ocslab.com:18011/LogGeneralizer --client-id=1\n"
  "\tLogGeneralizerAdmin get-stat-info"
  " -r corbaloc:iiop:srv-dev.ocslab.com:18011/LogGeneralizer --client-id=1"
  " --clear\n";

const Table::Column STAT_INFO_TABLE_COLUMNS[] =
{
  Table::Column("sdate", Table::Column::TEXT),
  Table::Column("adv_sdate", Table::Column::TEXT),
  Table::Column("adv_account_id", Table::Column::NUMBER),
  Table::Column("campaign_id", Table::Column::NUMBER),
  Table::Column("ccg_id", Table::Column::NUMBER),
  Table::Column("adv_account_amount", Table::Column::NUMBER),
  Table::Column("adv_amount", Table::Column::NUMBER),
  Table::Column("adv_comm_amount", Table::Column::NUMBER),
  Table::Column("adv_payable_comm_amount", Table::Column::NUMBER),
  Table::Column("creative_stats", Table::Column::TEXT),
  Table::Column("publisher_amounts", Table::Column::TEXT),
  Table::Column("tag_amounts", Table::Column::TEXT)
};

typedef std::unique_ptr<Table> TablePtr;

const char STOP_CMD[]     = "stop-stat-upload";
const char START_CMD[]    = "start-stat-upload";
const char GET_INFO_CMD[] = "get-stat-info";

} // namespace

int main(int argc, char **argv)
{
  Application *app = 0;

  try
  {
    app = Application::instance();
    return app->run(argc, argv);
  }
  catch (const Application::InvalidArgument &ex)
  {
    std::cerr << "Invalid argument. Exception caught:\n" << ex.what()
              << "\nRun 'LogGeneralizerAdmin help' for usage information\n";
  }
  catch (const Application::Exception &ex)
  {
    std::cerr << "LogGeneralizerAdmin: Application::Exception exception "
      "caught. :" << std::endl << ex.what() << std::endl;
  }
  catch (const eh::Exception &ex)
  {
    std::cerr << "LogGeneralizerAdmin: eh::Exception exception caught. "
      ":" << std::endl << ex.what() << std::endl;
  }
  catch (const CORBA::Exception &ex)
  {
    std::cerr << "LogGeneralizerAdmin: CORBA::Exception exception caught. "
      ":" << std::endl << ex << std::endl;
  }
  catch (...)
  {
    std::cerr << "LogGeneralizerAdmin: unknown exception caught" << std::endl;
  }

  return -1;
}

Application::Application() /*throw(Application::Exception, eh::Exception)*/
{
}

Application::~Application() noexcept
{
  log_generalizer_ = 0;

  if (!CORBA::is_nil(orb_))
  {
    orb_->destroy();
  }
}

int
Application::run(int &argc, char **argv)
  /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::Option<std::string> opt_ref;
  Generics::AppUtils::Option<unsigned> opt_id;
  Generics::AppUtils::CheckOption opt_clear;
  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
      Generics::AppUtils::short_name("h"),
    opt_help
  );

  args.add(
    Generics::AppUtils::equal_name("ref") ||
      Generics::AppUtils::short_name("r"),
    opt_ref
  );

  args.add(
    Generics::AppUtils::equal_name("client-id") ||
      Generics::AppUtils::short_name("i"),
    opt_id
  );

  args.add(
    Generics::AppUtils::equal_name("clear") ||
      Generics::AppUtils::short_name("c"),
    opt_clear
  );

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

  const Generics::AppUtils::Args::CommandList &commands = args.commands();

  if (commands.empty() || opt_help.enabled() || *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  std::string cmd = *commands.begin();

  if (cmd == STOP_CMD || cmd == START_CMD || cmd == GET_INFO_CMD)
  {
    if (!opt_ref.installed())
    {
      std::cout << "LogGeneralizer reference isn't defined." << std::endl
                << USAGE << std::endl;
      return 1;
    }

    {
      std::string service_ref = *opt_ref;
      unsigned client_id = *opt_id;
      bool clear = opt_clear.enabled();

      try
      {
        CORBA::Object_var obj = orb_->string_to_object(service_ref.c_str());

        if (CORBA::is_nil(obj))
        {
          Stream::Error es;
          es << "string_to_object() failed for service reference '"
             << service_ref << "'";
          throw Exception(es);
        }

        log_generalizer_ =
          AdServer::LogProcessing::LogGeneralizer::_narrow(obj.in());

        if (CORBA::is_nil(log_generalizer_))
        {
          Stream::Error es;
          es << "AdServer::LogProcessing::LogGeneralizer::_narrow() failed "
             "for service reference '" << service_ref << "'";
          throw Exception(es);
        }
      }
      catch (const CORBA::SystemException &ex)
      {
        Stream::Error es;
        es << "Failed to resolve service reference '" << service_ref
           << "'. CORBA::SystemException caught:\n" << ex;
        throw Exception(es);
      }

      try
      {
        if (cmd == STOP_CMD)
        {
          log_generalizer_->stop_stat_upload(client_id);
        }
        else if (cmd == START_CMD)
        {
          log_generalizer_->start_stat_upload(client_id, clear);
        }
        else if (cmd == GET_INFO_CMD)
        {
          StatInfo_var info =
            log_generalizer_->get_stat_info(client_id, clear);
          print_stat_info(info);
        }
      }
      catch (const CORBA::SystemException &ex)
      {
        Stream::Error es;
        es << "Operation failed: CORBA::SystemException caught:\n" << ex;
        throw Exception(es);
      }
      catch (const LogGeneralizerNotSupported &ex)
      {
        Stream::Error es;
        es << "Operation failed: LogGeneralizer::NotSupported "
           << "caught:\n" << ex.description;
        throw Exception(es);
      }
      catch (const LogGeneralizerImplementationException &ex)
      {
        Stream::Error es;
        es << "Operation failed: LogGeneralizer::ImplementationException "
           << "caught:\n" << ex.description;
        throw Exception(es);
      }
      catch (const LogGeneralizerCollectionNotStarted &ex)
      {
        Stream::Error es;
        es << "Operation failed: LogGeneralizer::CollectionNotStarted "
           << "caught:\n" << ex.description;
        throw Exception(es);
      }
    }
  }
  else
  {
    Stream::Error es;
    es << "Unknown command '" << cmd << "'";
    throw InvalidArgument(es);
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

void
Application::output_creative_stats(
  std::ostream &os,
  const CreativeStatSeq &creative_stats
)
  /*throw(eh::Exception, CORBA::Exception)*/
{
  for (size_t i = 0; i < creative_stats.length(); ++i)
  {
    if (i)
    {
      os << ", ";
    }
    const CreativeStatInfo &csi = creative_stats[i];
    os << "[ " << csi.cc_id << " : " << csi.requests << ", "
       << csi.impressions << ", " << csi.clicks << ", "
       << csi.actions << " ]";
  }
}

void
Application::output_publisher_amounts(
  std::ostream &os,
  const PublisherAmountSeq &publisher_amounts
)
  /*throw(eh::Exception, CORBA::Exception)*/
{
  typedef Generics::SimpleDecimal<uint64_t, 18, 8> DecimalT;
  for (size_t i = 0; i < publisher_amounts.length(); ++i)
  {
    if (i)
    {
      os << ", ";
    }
    const PublisherAmountInfo &pai = publisher_amounts[i];
    DecimalT adv_amount = CorbaAlgs::unpack_decimal<DecimalT>(pai.adv_amount);
    os << "[ " << pai.publisher_account_id << " : "
       << adv_amount << " ]";
  }
}

void
Application::output_tag_amounts(
  std::ostream &os,
  const TagAmountSeq &tag_amounts
)
  /*throw(eh::Exception, CORBA::Exception)*/
{
  typedef Generics::SimpleDecimal<uint64_t, 18, 8> DecimalT;
  for (size_t i = 0; i < tag_amounts.length(); ++i)
  {
    if (i)
    {
      os << ", ";
    }
    const TagAmountInfo &tai = tag_amounts[i];
    DecimalT pub_isp_amount =
      CorbaAlgs::unpack_decimal<DecimalT>(tai.pub_isp_amount);
    DecimalT adv_amount = CorbaAlgs::unpack_decimal<DecimalT>(tai.adv_amount);
    DecimalT adv_comm_amount =
      CorbaAlgs::unpack_decimal<DecimalT>(tai.adv_comm_amount);
    os << "[ " << tai.tag_id << " : " << pub_isp_amount << ", "
       << adv_amount << ", " << adv_comm_amount << " ]";
  }
}

void
Application::print_stat_info(const StatInfo &info)
  /*throw(eh::Exception, CORBA::Exception)*/
{
  const size_t columns = sizeof(STAT_INFO_TABLE_COLUMNS) /
    sizeof(STAT_INFO_TABLE_COLUMNS[0]);
  TablePtr table(new Table(columns));
  for (size_t i = 0; i < columns; ++i)
  {
    table->column(i, STAT_INFO_TABLE_COLUMNS[i]);
  }
  typedef Generics::SimpleDecimal<uint64_t, 18, 8> DecimalT;
  using namespace AdServer::LogProcessing;
  const CampaignStatSeq &cmp_stat_seq = info.campaign_stats;
  for (size_t i = 0; i < cmp_stat_seq.length(); ++i)
  {
    Table::Row row(table->columns());
    const Generics::Time sdate =
      CorbaAlgs::unpack_time(cmp_stat_seq[i].sdate);
    row.add_field(sdate.get_gm_time().format("%F %H:%M"));
    const Generics::Time adv_sdate =
      CorbaAlgs::unpack_time(cmp_stat_seq[i].adv_sdate);
    row.add_field(adv_sdate.get_gm_time().format("%F %H:%M"));
    row.add_field(cmp_stat_seq[i].adv_account_id);
    row.add_field(cmp_stat_seq[i].campaign_id);
    row.add_field(cmp_stat_seq[i].ccg_id);
    const DecimalT adv_account_amount =
      CorbaAlgs::unpack_decimal<DecimalT>(cmp_stat_seq[i].adv_account_amount);
    row.add_field(adv_account_amount.str());
    const DecimalT adv_amount =
      CorbaAlgs::unpack_decimal<DecimalT>(cmp_stat_seq[i].adv_amount);
    row.add_field(adv_amount.str());
    const DecimalT adv_comm_amount =
      CorbaAlgs::unpack_decimal<DecimalT>(cmp_stat_seq[i].adv_comm_amount);
    row.add_field(adv_comm_amount.str());
    const DecimalT adv_payable_comm_amount =
      CorbaAlgs::unpack_decimal<DecimalT>(
        cmp_stat_seq[i].adv_payable_comm_amount
      );
    row.add_field(adv_payable_comm_amount.str());
    {
      std::ostringstream oss;
      output_creative_stats(oss, cmp_stat_seq[i].creative_stats);
      row.add_field(oss.str());
    }
    {
      std::ostringstream oss;
      output_publisher_amounts(oss, cmp_stat_seq[i].publisher_amounts);
      row.add_field(oss.str());
    }
    {
      std::ostringstream oss;
      output_tag_amounts(oss, cmp_stat_seq[i].tag_amounts);
      row.add_field(oss.str());
    }
    table->add_row(row);
  }
  std::cout << std::endl;
  std::cout << "Number of records: " << cmp_stat_seq.length() << std::endl;
  std::cout << std::endl;
  table->dump(std::cout);
}

