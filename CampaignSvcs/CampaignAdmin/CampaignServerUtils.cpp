#include <Commons/CorbaAlgs.hpp>

#include "Application.hpp"

/*
 * CampaignServer manipulation methods
 */
namespace
{
  const char TIME_FORMAT[] = "%d-%m-%Y:%H-%M-%S.%q";
  const char DATE_FORMAT[] = "%d-%m-%Y";

  const Table::Column BILL_ACCOUNT_COLUMNS[] =
  {
    Table::Column("account_id", Table::Column::NUMBER),
    Table::Column("day", Table::Column::TEXT),
    Table::Column("amount", Table::Column::TEXT)
  };

  const Table::Column BILL_CAMPAIGN_COLUMNS[] =
  {
    Table::Column("campaign_id", Table::Column::NUMBER),
    Table::Column("day", Table::Column::TEXT),
    Table::Column("amount", Table::Column::TEXT),
    Table::Column("imps", Table::Column::TEXT),
    Table::Column("clicks", Table::Column::TEXT)
  };

  const Table::Column BILL_CCG_COLUMNS[] =
  {
    Table::Column("ccg_id", Table::Column::NUMBER),
    Table::Column("day", Table::Column::TEXT),
    Table::Column("amount", Table::Column::TEXT),
    Table::Column("imps", Table::Column::TEXT),
    Table::Column("clicks", Table::Column::TEXT)
  };

  const Table::Column SIMPLE_CHANNEL_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("country_code", Table::Column::TEXT),
    Table::Column("language", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("behav_param_list_id", Table::Column::TEXT),
    Table::Column("categories", Table::Column::TEXT),
    Table::Column("threshold", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column SIMPLE_CHANNEL_CS_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("country_code", Table::Column::TEXT),
    Table::Column("language", Table::Column::TEXT),
    Table::Column("channel_type", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("behave_info", Table::Column::TEXT)
  };

  const Table::Column BEHAV_PARAM_TABLE_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("threshold", Table::Column::NUMBER),
    Table::Column("minimum_visits", Table::Column::NUMBER),
    Table::Column("time_from", Table::Column::NUMBER),
    Table::Column("time_to", Table::Column::NUMBER),
    Table::Column("weight", Table::Column::NUMBER),
    Table::Column("trigger_type", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column KEY_BEHAV_PARAM_TABLE_COLUMNS[] =
  {
    Table::Column("key", Table::Column::TEXT),
    Table::Column("threshold", Table::Column::NUMBER),
    Table::Column("minimum_visits", Table::Column::NUMBER),
    Table::Column("time_from", Table::Column::NUMBER),
    Table::Column("time_to", Table::Column::NUMBER),
    Table::Column("weight", Table::Column::NUMBER),
    Table::Column("trigger_type", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column FRAUD_CONDITION_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("type", Table::Column::TEXT),
    Table::Column("period", Table::Column::NUMBER),
    Table::Column("limit", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column SEARCH_ENGINES_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("host_postfix", Table::Column::TEXT),
    Table::Column("regexp", Table::Column::TEXT),
    Table::Column("encoding", Table::Column::TEXT),
    Table::Column("post_encoding", Table::Column::TEXT),
    Table::Column("decoding_depth", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column WEB_BROWSER_DETECTORS_COLUMNS[] =
  {
    Table::Column("name", Table::Column::TEXT),
    Table::Column("marker", Table::Column::TEXT),
    Table::Column("regexp", Table::Column::TEXT),
    Table::Column("regexp_required", Table::Column::NUMBER),
    Table::Column("priority", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column PLATFORM_DETECTORS_COLUMNS[] =
  {
    Table::Column("platform_id", Table::Column::NUMBER),
    Table::Column("name", Table::Column::TEXT),
    Table::Column("type", Table::Column::TEXT),
    Table::Column("use_name", Table::Column::TEXT),
    Table::Column("marker", Table::Column::TEXT),
    Table::Column("match_regexp", Table::Column::TEXT),
    Table::Column("output_regexp", Table::Column::TEXT),
    Table::Column("priority", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column COLOCATION_FLAGS_COLUMNS[] =
  {
    Table::Column("colo_id", Table::Column::NUMBER),
    Table::Column("flags", Table::Column::NUMBER),
  };

  const Table::Column DELETED_KEY_COLUMNS[] =
  {
    Table::Column("key", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column EXPRESSION_CHANNEL_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("type", Table::Column::TEXT),
    Table::Column("country", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("action_id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),

    Table::Column("name", Table::Column::TEXT),
    Table::Column("parent_channel_id", Table::Column::NUMBER),

    Table::Column("account_id", Table::Column::NUMBER),
    Table::Column("public", Table::Column::NUMBER),
    Table::Column("freq_cap_id", Table::Column::NUMBER),
    Table::Column("language", Table::Column::TEXT),

    Table::Column("expression", Table::Column::TEXT),
    Table::Column("discover_query", Table::Column::TEXT),
    Table::Column("discover_annotation", Table::Column::TEXT),
    Table::Column("create_time", Table::Column::TEXT),

    Table::Column("channel_rate_id", Table::Column::NUMBER),
    Table::Column("imp_revenue", Table::Column::TEXT),
    Table::Column("click_revenue", Table::Column::TEXT),
  };

  const Table::Column STAT_CCG_COLUMNS[] =
  {
    Table::Column("ccg_id", Table::Column::NUMBER),
    Table::Column("campaign_id", Table::Column::NUMBER),
    Table::Column("impressions", Table::Column::NUMBER),
    Table::Column("clicks", Table::Column::NUMBER),
    Table::Column("actions", Table::Column::NUMBER),
    Table::Column("amount", Table::Column::TEXT),
    Table::Column("comm_amount", Table::Column::TEXT),
    Table::Column("daily_amount", Table::Column::TEXT),
    Table::Column("daily_comm_amount", Table::Column::TEXT),
    Table::Column("hourly_amount", Table::Column::TEXT),
    Table::Column("hourly_comm_amount", Table::Column::TEXT),    
    Table::Column("creatives", Table::Column::TEXT),
    Table::Column("publishers", Table::Column::TEXT),
    Table::Column("tags", Table::Column::TEXT),
    Table::Column("ctr_resets", Table::Column::TEXT),
  };

  const Table::Column STAT_CAMPAIGN_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("amount", Table::Column::TEXT),
    Table::Column("comm_amount", Table::Column::TEXT),
    Table::Column("daily_amount", Table::Column::TEXT),
    Table::Column("daily_comm_amount", Table::Column::TEXT),
  };

  const Table::Column STAT_AMOUNT_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("amount", Table::Column::TEXT),
    Table::Column("comm_amount", Table::Column::TEXT),
    Table::Column("daily_amount", Table::Column::TEXT),
    Table::Column("daily_comm_amount", Table::Column::TEXT),
  };
}

template<class SequenceType>
void print_deleted_(
  const SequenceType& seq, const Table::Column legend[], size_t columns)
  noexcept
{
  TablePtr table(Application::setup_table_(columns, legend));
  for(CORBA::ULong i = 0; i < seq.length(); i++)
  {
    Table::Row row(table->columns());
    row.add_field(CorbaAlgs::convert_type_adapter(seq[i].id));
    row.add_field(CorbaAlgs::unpack_time(
      seq[i].timestamp).get_gm_time().format(TIME_FORMAT));
    table->add_row(row);
  }
  table->dump(std::cout);
}

Table* Application::setup_table_(
  size_t columns,
  const Table::Column legend[])
  noexcept
{
  TablePtr table;
  table = TablePtr(new Table(columns));
  for(size_t i = 0; i < columns; i++)
  {
    table->column(i, legend[i]);
  }
  return table.release();
}

void add_amount_distr_info_(
  Table* table,
  CORBA::ULong id,
  const AdServer::CampaignSvcs_v360::AmountDistributionInfo& amounts)
  noexcept
{
  {
    Table::Row row(table->columns());
    row.add_field(id);
    std::string day_prev = CorbaAlgs::unpack_time(
      amounts.prev_days_amount.day).get_gm_time().format(DATE_FORMAT);
    std::string amount_prev =
      CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
        amounts.prev_days_amount.amount).str();
    row.add_field(day_prev + " (prev)");
    row.add_field(amount_prev + " (prev)");
    table->add_row(row);
  }
  for(CORBA::ULong i = 0; i < amounts.day_amounts.length(); ++i)
  {
    const AdServer::CampaignSvcs::DayAmount& day = amounts.day_amounts[i];
    Table::Row row(table->columns());
    row.add_field(id);
    row.add_field(
      CorbaAlgs::unpack_time(day.day).get_gm_time().format(DATE_FORMAT));
    row.add_field(
      CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(day.amount).str());
    table->add_row(row);
  }
}

void add_amount_count_distr_info_(
  Table* table,
  CORBA::ULong id,
  const AdServer::CampaignSvcs_v360::AmountCountDistributionInfo& amounts)
  noexcept
{
  {
    Table::Row row(table->columns());
    row.add_field(id);
    std::string day_prev = CorbaAlgs::unpack_time(
      amounts.prev_days_amount_count.day).get_gm_time().format(DATE_FORMAT);
    std::string amount_prev =
      CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
        amounts.prev_days_amount_count.amount).str();
    row.add_field(day_prev + " (prev)");
    row.add_field(amount_prev + " (prev)");
    row.add_field(CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::ImpRevenueDecimal>(
      amounts.prev_days_amount_count.imps).str() + " (prev)");
    row.add_field(CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::ImpRevenueDecimal>(
      amounts.prev_days_amount_count.clicks).str() + " (prev)");
    table->add_row(row);
  }
  for(CORBA::ULong i = 0; i < amounts.day_amount_counts.length(); ++i)
  {
    const AdServer::CampaignSvcs::DayAmountCount& day = amounts.day_amount_counts[i];
    Table::Row row(table->columns());
    row.add_field(id);
    row.add_field(
      CorbaAlgs::unpack_time(day.day).get_gm_time().format(DATE_FORMAT));
    row.add_field(
      CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(day.amount).str());
    row.add_field(
      CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::ImpRevenueDecimal>(day.imps).str());
    row.add_field(
      CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::ImpRevenueDecimal>(day.clicks).str());
    table->add_row(row);
  }
}

int Application::bill_stats(int command_index)
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  init_server_ref_();
  try
  {
    AdServer::CampaignSvcs::BillStatInfo_var bill_stat =
      server_->get_bill_stat();

    TablePtr table;
    switch(command_index)
    {
      case command_bill_accounts:
        table.reset(setup_table_(
          sizeof(BILL_ACCOUNT_COLUMNS) / sizeof(BILL_ACCOUNT_COLUMNS[0]),
          BILL_ACCOUNT_COLUMNS));
        for(CORBA::ULong i = 0; i < bill_stat->accounts.length(); ++i)
        {
          add_amount_distr_info_(
            table.get(),
            bill_stat->accounts[i].account_id,
            bill_stat->accounts[i].amount_distribution);
        }
        break;
      case command_bill_campaigns:
        table.reset(setup_table_(
          sizeof(BILL_CAMPAIGN_COLUMNS) / sizeof(BILL_CAMPAIGN_COLUMNS[0]),
          BILL_CAMPAIGN_COLUMNS));
        for(CORBA::ULong i = 0; i < bill_stat->campaigns.length(); ++i)
        {
          add_amount_count_distr_info_(
            table.get(),
            bill_stat->campaigns[i].campaign_id,
            bill_stat->campaigns[i].amount_count_distribution);
        }
        break;
      case command_bill_ccgs:
        table.reset(setup_table_(
          sizeof(BILL_CCG_COLUMNS) / sizeof(BILL_CCG_COLUMNS[0]),
          BILL_CCG_COLUMNS));
        for(CORBA::ULong i = 0; i < bill_stat->ccgs.length(); ++i)
        {
          add_amount_count_distr_info_(
            table.get(),
            bill_stat->ccgs[i].ccg_id,
            bill_stat->ccgs[i].amount_count_distribution);
        }
        break;
      default:
        throw InvalidArgument("unknown command");
        break;
    }
    table->dump(std::cout);
    return 0;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& ex)
  {
    std::cerr << __func__ << ": caught CampaignServer::NotReady: " <<
      ex.description << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
  {
    std::cerr << __func__ << ": caught CampaignServer::ImplementationException: " <<
      ex.description << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << __func__ << ": caught CORBA::SystemException: " << ex << std::endl;
  }
  return 1;
}

void Application::simple_channel_cs()
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  const unsigned long PORTIONS_NUMBER = 20;

  init_server_ref_();
  try
  {
    AdServer::CampaignSvcs::ChannelServerChannelAnswer_var simple_answer;
    for(unsigned long portion = 0; portion < PORTIONS_NUMBER; ++portion)
    {
      AdServer::CampaignSvcs::CampaignServer::GetSimpleChannelsInfo config_settings;
      config_settings.portion = portion;
      config_settings.portions_number = PORTIONS_NUMBER;
      config_settings.channel_statuses << String::SubString("AIW");

      simple_answer = server_->chsv_simple_channels(config_settings);

      const AdServer::CampaignSvcs::CSSimpleChannelSeq& channels =
        simple_answer->simple_channels;

      unsigned long columns =
        sizeof(SIMPLE_CHANNEL_CS_COLUMNS) / sizeof(SIMPLE_CHANNEL_CS_COLUMNS[0]);

      TablePtr table = TablePtr(new Table(columns));

      for(unsigned long j = 0; j < columns; ++j)
      {
        table->column(j, SIMPLE_CHANNEL_CS_COLUMNS[j]);
      }

      Table::Row row(table->columns());
      for(unsigned long i = 0; i < channels.length(); ++i)
      {
        const AdServer::CampaignSvcs::CSSimpleChannel& simple_channel =
          channels[i];

        row.add_field(simple_channel.channel_id);
        row.add_field(simple_channel.country_code);
        row.add_field(simple_channel.language);
        row.add_field(simple_channel.channel_type);
        row.add_field(simple_channel.status);
        {
          std::ostringstream behave_stream;
          for(CORBA::ULong j = 0; j < simple_channel.behave_info.length(); ++j)
          {
            if ( j != 0)
            {
              behave_stream << ", ";
            }
            behave_stream << simple_channel.behave_info[j].trigger_type;
            if (simple_channel.behave_info[j].is_context)
            {
              behave_stream << simple_channel.behave_info[j].weight;
            }
          }
          row.add_field(behave_stream.str());
        }
        table->add_row(row, filters_, sorter_);
        row.clear();
      }

      table->dump(std::cout);
    }
    std::cout << "cost_limit : " <<
      CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
        simple_answer->cost_limit).str() << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& ex)
  {
    std::cerr << __func__ << ": caught CampaignServer::NotReady: " <<
      ex.description << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
  {
    std::cerr << __func__ << ": caught CampaignServer::ImplementationException: " <<
      ex.description << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << __func__ << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void Application::simple_channel()
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  static const char* FUN = "Application::simple_channel()";

  const unsigned long PORTIONS_NUMBER = 20;

  init_server_ref_();
  try
  {
    for(unsigned long portion = 0; portion < PORTIONS_NUMBER; ++portion)
    {
      AdServer::CampaignSvcs::CampaignServer::GetSimpleChannelsInfo config_settings;
      config_settings.portion = portion;
      config_settings.portions_number = PORTIONS_NUMBER;

      AdServer::CampaignSvcs::SimpleChannelAnswer_var simple_answer =
        server_->simple_channels(config_settings);

      const AdServer::CampaignSvcs::SimpleChannelKeySeq& channels =
        simple_answer->simple_channels;

      unsigned long columns =
        sizeof(SIMPLE_CHANNEL_COLUMNS) / sizeof(SIMPLE_CHANNEL_COLUMNS[0]);

      TablePtr table = TablePtr(new Table(columns));

      for(unsigned long j = 0; j < columns; ++j)
      {
        table->column(j, SIMPLE_CHANNEL_COLUMNS[j]);
      }

      Table::Row row(table->columns());
      for(unsigned long i = 0; i < channels.length(); ++i)
      {
        const AdServer::CampaignSvcs::SimpleChannelKey& simple_channel =
          channels[i];

        row.add_field(simple_channel.channel_id);
        row.add_field(simple_channel.country_code);
        row.add_field(simple_channel.language);
        row.add_field(simple_channel.status);
        if (*simple_channel.str_behav_param_list_id)
        {
          row.add_field(simple_channel.str_behav_param_list_id);
        }
        else
        {
          Stream::Error ostr;
          ostr << simple_channel.behav_param_list_id;
          row.add_field(ostr.str());
        }
        row.add_field(sequence_to_string(simple_channel.categories));
        row.add_field(simple_channel.threshold);
        row.add_field(CorbaAlgs::unpack_time(
          simple_channel.timestamp).get_gm_time().format(TIME_FORMAT));
        table->add_row(row, filters_, sorter_);
        row.clear();
      }

      table->dump(std::cout);
    }
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::NotReady: " <<
      ex.description << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::ImplementationException: " <<
      ex.description << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void Application::print_search_engines_(
  const AdServer::CampaignSvcs::SearchEngineSeq& engines,
  Table::Filters& filters) noexcept
{
  size_t columns = sizeof(SEARCH_ENGINES_COLUMNS) /
    sizeof(SEARCH_ENGINES_COLUMNS[0]);
  TablePtr table = TablePtr(new Table(columns));
  for(size_t i = 0; i < columns; i++)
  {
    table->column(i, SEARCH_ENGINES_COLUMNS[i]);
  }
  for(CORBA::ULong i = 0; i < engines.length(); i++)
  {
    const AdServer::CampaignSvcs::SearchEngineInfo& engine = engines[i];
    for(size_t j = 0; j < engine.regexps.length(); j++)
    {
      Table::Row row(table->columns());
      row.add_field(engine.id);
      row.add_field(engine.regexps[j].host_postfix);
      row.add_field(engine.regexps[j].regexp);
      row.add_field(engine.regexps[j].encoding);
      row.add_field(engine.regexps[j].post_encoding);
      row.add_field(engine.regexps[j].decoding_depth);
      row.add_field(CorbaAlgs::unpack_time(
        engine.timestamp).get_gm_time().format(TIME_FORMAT));
      table->add_row(row, filters);
    }
  }
  table->dump(std::cout);
}

void Application::print_colocation_flags_(
  const AdServer::CampaignSvcs::ColocationFlagsSeq& colocations)
  noexcept
{
  size_t columns = sizeof(COLOCATION_FLAGS_COLUMNS) /
    sizeof(COLOCATION_FLAGS_COLUMNS[0]);
  TablePtr table = TablePtr(new Table(columns));
  for (size_t i = 0; i < columns; ++i)
  {
    table->column(i, COLOCATION_FLAGS_COLUMNS[i]);
  }
  Table::Filters filters;
  for (CORBA::ULong i = 0; i < colocations.length(); ++i)
  {
    const AdServer::CampaignSvcs::ColocationFlags& colocation = colocations[i];
    Table::Row row(table->columns());
    row.add_field(colocation.colo_id);
    row.add_field(colocation.flags);
    table->add_row(row, filters);
  }
  table->dump(std::cout);
}

void Application::search_engines(bool use_update, bool deleted)
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  static const char* FUN = "Application::search_engines()";
  init_server_ref_();
  try
  {
    if(use_update)
    {
      AdServer::CampaignSvcs::CampaignGetConfigSettings config_settings;
      init_config_settings(config_settings);
      config_settings.no_deleted = false;
      AdServer::CampaignSvcs::CampaignConfigUpdateInfo_var config =
        server_->get_config(config_settings);
      if(deleted)
      {
        print_deleted_(
          config->deleted_search_engines,
          DELETED_KEY_COLUMNS,
          sizeof(DELETED_KEY_COLUMNS)/sizeof(DELETED_KEY_COLUMNS[0]));
      }
      else
      {
        print_search_engines_(config->search_engines, filters_);
      }
    }
    else
    {
      AdServer::CampaignSvcs::DetectorsConfig_var detectors =
        server_->detectors(
          CorbaAlgs::pack_time(Generics::Time::ZERO));
      print_search_engines_(detectors->engines, filters_);
    }
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::NotReady: " <<
      ex.description << std::endl;
  }
  catch(const AdServer::CampaignSvcs::
        CampaignServer::ImplementationException& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::ImplementationException: " <<
      ex.description << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void Application::detectors(bool web, bool platform)
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  static const char* FUN = "Application::detectors()";
  init_server_ref_();
  try
  {
    AdServer::CampaignSvcs::DetectorsConfig_var detectors =
      server_->detectors(
        CorbaAlgs::pack_time(Generics::Time::ZERO));

    if(web)
    {
      // web browser detectors
      size_t columns = sizeof(WEB_BROWSER_DETECTORS_COLUMNS) /
        sizeof(WEB_BROWSER_DETECTORS_COLUMNS[0]);
      TablePtr table = TablePtr(new Table(columns));
      for(size_t i = 0; i < columns; i++)
      {
        table->column(i, WEB_BROWSER_DETECTORS_COLUMNS[i]);
      }

      for(CORBA::ULong i = 0; i < detectors->web_browsers.length(); i++)
      {
        const AdServer::CampaignSvcs::WebBrowserInfo& wb =
          detectors->web_browsers[i];
        for(size_t j = 0; j < wb.detectors.length(); j++)
        {
          Table::Row row(table->columns());
          row.add_field(wb.name);
          row.add_field(wb.detectors[j].marker);
          row.add_field(wb.detectors[j].regexp);
          row.add_field(wb.detectors[j].regexp_required);
          row.add_field(wb.detectors[j].priority);
          row.add_field(CorbaAlgs::unpack_time(
            wb.timestamp).get_gm_time().format(TIME_FORMAT));
          table->add_row(row, filters_, sorter_);
        }
      }
      table->dump(std::cout);
    }

    if(platform)
    {
      size_t columns = sizeof(PLATFORM_DETECTORS_COLUMNS) /
        sizeof(PLATFORM_DETECTORS_COLUMNS[0]);
      TablePtr table = TablePtr(new Table(columns));
      for(size_t i = 0; i < columns; i++)
      {
        table->column(i, PLATFORM_DETECTORS_COLUMNS[i]);
      }

      for(CORBA::ULong i = 0; i < detectors->platforms.length(); i++)
      {
        const AdServer::CampaignSvcs::PlatformInfo& platform =
          detectors->platforms[i];
        if (!platform.detectors.length())
        {
          Table::Row row(table->columns());
          row.add_field(platform.platform_id);
          row.add_field(platform.name);
          row.add_field(platform.type);
          // Use empty values for detectors fields
          row.resize(table->columns() - 1);
          row.add_field(CorbaAlgs::unpack_time(
            platform.timestamp).get_gm_time().format(TIME_FORMAT));
          table->add_row(row, filters_, sorter_);
        }
        else
        {
          for(size_t j = 0; j < platform.detectors.length(); j++)
          {
            Table::Row row(table->columns());
            row.add_field(platform.platform_id);
            row.add_field(platform.name);
            row.add_field(platform.type);
            row.add_field(platform.detectors[j].use_name);
            row.add_field(platform.detectors[j].marker);
            row.add_field(platform.detectors[j].match_regexp);
            row.add_field(platform.detectors[j].output_regexp);
            row.add_field(platform.detectors[j].priority);
            row.add_field(CorbaAlgs::unpack_time(
              platform.timestamp).get_gm_time().format(TIME_FORMAT));
            table->add_row(row, filters_, sorter_);
          }
        }
      }
      table->dump(std::cout);
    }
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::NotReady: " <<
      ex.description << std::endl;
  }
  catch(const AdServer::CampaignSvcs::
        CampaignServer::ImplementationException& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::ImplementationException: " <<
      ex.description << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void Application::fraud_conditions()
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  static const char* FUN = "Application::fraud_conditions()";

  init_server_ref_();
  try
  {
    AdServer::CampaignSvcs::FraudConditionConfig_var fraud_config =
      server_->fraud_conditions();

    const AdServer::CampaignSvcs::FraudConditionSeq& fraud_conds =
      fraud_config->rules;

    unsigned long columns =
      sizeof(FRAUD_CONDITION_COLUMNS) / sizeof(FRAUD_CONDITION_COLUMNS[0]);

    TablePtr table = TablePtr(new Table(columns));

    for(unsigned long j = 0; j < columns; ++j)
    {
      table->column(j, FRAUD_CONDITION_COLUMNS[j]);
    }

    for(unsigned long i = 0; i < fraud_conds.length(); ++i)
    {
      const AdServer::CampaignSvcs::FraudConditionInfo& fraud_cond =
        fraud_conds[i];

      Table::Row row(table->columns());
      row.add_field(fraud_cond.id);
      row.add_field(fraud_cond.type);
      row.add_field(CorbaAlgs::unpack_time(fraud_cond.period).tv_sec);
      row.add_field(fraud_cond.limit);
      row.add_field(CorbaAlgs::unpack_time(fraud_cond.timestamp).gm_ft());
      table->add_row(row, filters_, sorter_);
    }

    table->dump(std::cout);
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::NotReady: " <<
      ex.description << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::ImplementationException: " <<
      ex.description << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void Application::init_config_settings(
  AdServer::CampaignSvcs::CampaignGetConfigSettings& config_settings)
  noexcept
{
  config_settings.timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
  config_settings.provide_only_tags = false;
  config_settings.portion = 0;
  config_settings.portions_number = 1;
  config_settings.colo_id = 0;
  config_settings.no_deleted = true;
  config_settings.geo_channels_timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
}

template<typename VALUE>
void Application::add_behav_value_(Table* table, const VALUE& info) noexcept
{
  std::string timestamp = CorbaAlgs::unpack_time(
    info.timestamp).get_gm_time().format(TIME_FORMAT);
  for(CORBA::ULong j = 0; j < info.bp_seq.length(); j++)
  {
    Table::Row row(table->columns());
    row.add_field(info.id);
    row.add_field(info.threshold);
    row.add_field(info.bp_seq[j].min_visits);
    row.add_field(info.bp_seq[j].time_from);
    row.add_field(info.bp_seq[j].time_to);
    row.add_field(info.bp_seq[j].weight);
    row.add_field(info.bp_seq[j].trigger_type);
    row.add_field(timestamp);
    table->add_row(row, filters_, sorter_);
  }
}

void Application::behav_param(int command_index)
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  static const char* FUN = "Application::behav_param()";

  init_server_ref_();
  try
  {
    AdServer::CampaignSvcs::CampaignGetConfigSettings config_settings;
    init_config_settings(config_settings);
    AdServer::CampaignSvcs::CampaignConfigUpdateInfo_var config =
      server_->get_config(config_settings);

    TablePtr table;
    unsigned long columns = 0;
    const Table::Column* columns_ptr = 0;
    switch(command_index)
    {
      case command_behave_param:
        columns_ptr = BEHAV_PARAM_TABLE_COLUMNS;
        columns = sizeof(BEHAV_PARAM_TABLE_COLUMNS)
          / sizeof(BEHAV_PARAM_TABLE_COLUMNS[0]);
        break;
      case command_behave_keys:
        columns_ptr = KEY_BEHAV_PARAM_TABLE_COLUMNS;
        columns = sizeof(KEY_BEHAV_PARAM_TABLE_COLUMNS)
          / sizeof(KEY_BEHAV_PARAM_TABLE_COLUMNS[0]);
        break;
    }
    table = TablePtr(new Table(columns));

    for(unsigned long i = 0; i < columns; i++)
    {
      table->column(i, columns_ptr[i]);
    }
    if(command_index == command_behave_param)
    {
      for(CORBA::ULong i = 0; i < config->behav_params.length(); i++)
      {
        const AdServer::CampaignSvcs::BehavParamInfo& info =
          config->behav_params[i];
        add_behav_value_(table.get(), info);
      }
    }
    else if(command_index == command_behave_keys)
    {
      for(CORBA::ULong i = 0; i < config->key_behav_params.length(); i++)
      {
        const AdServer::CampaignSvcs::KeyBehavParamInfo& info =
          config->key_behav_params[i];
        add_behav_value_(table.get(), info);
      }
    }
    else
    {
      throw InvalidArgument("Invalid command");
    }
    table->dump(std::cout);
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::NotReady: " <<
      ex.description << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
  {
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void Application::expression_channels()
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  static const char* FUN = "Application::expression_channels()";
  const unsigned long PORTIONS_NUMBER = 20;

  init_server_ref_();
  try
  {
    AdServer::CampaignSvcs::ExpressionChannelHolderMap channel_holders;

    AdServer::CampaignSvcs::CampaignServer::GetExpressionChannelsInfo request_settings;
    request_settings.timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
    request_settings.channel_statuses = "AIWP";
    request_settings.portions_number = PORTIONS_NUMBER;
    request_settings.provide_ccg_links = false;
    request_settings.provide_channel_triggers = false;

    for(unsigned long portion = 0; portion < PORTIONS_NUMBER; ++portion)
    {
      request_settings.portion = portion;
      AdServer::CampaignSvcs::ExpressionChannelsInfo_var channels_info =
        server_->get_expression_channels(request_settings);

      fill_channels_(channel_holders, channels_info->expression_channels);
    }

    unsigned long columns = sizeof(EXPRESSION_CHANNEL_COLUMNS) /
      sizeof(EXPRESSION_CHANNEL_COLUMNS[0]);

    TablePtr table = TablePtr(new Table(columns));

    for(unsigned long j = 0; j < columns; ++j)
    {
      table->column(j, EXPRESSION_CHANNEL_COLUMNS[j]);
    }


    for(AdServer::CampaignSvcs::ExpressionChannelHolderMap::const_iterator ch_it =
          channel_holders.begin();
        ch_it != channel_holders.end(); ++ch_it)
    {
      if(ch_it->second->channel.in())
      {
        Table::Row row(table->columns());
        describe_expression_channel(row, ch_it->second);
        table->add_row(row, filters_, sorter_);
      }
    }

    table->dump(std::cout);
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady&)
  {
    std::cerr << FUN << ": caught CampaignServer::NotReady" << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
  {
    std::cerr << FUN << ": caught CampaignServer::ImplementationException: " <<
      ex.description << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void
Application::campaign_stat()
  /*throw(Exception, InvalidArgument)*/
{
  static const char* FUN = "Application::campaign_stat()";

  init_server_ref_();
  try
  {
    AdServer::CampaignSvcs::StatInfo_var stat =
      server_->get_stat();

    unsigned long columns = sizeof(STAT_CAMPAIGN_COLUMNS) /
      sizeof(STAT_CAMPAIGN_COLUMNS[0]);

    TablePtr table = TablePtr(new Table(columns));

    for(unsigned long j = 0; j < columns; ++j)
    {
      table->column(j, STAT_CAMPAIGN_COLUMNS[j]);
    }

    const AdServer::CampaignSvcs::CampaignStatSeq& campaign_stats =
      stat->campaigns;

    for(CORBA::ULong i = 0; i < campaign_stats.length(); ++i)
    {
      const AdServer::CampaignSvcs::CampaignStatInfo& stat = campaign_stats[i];

      Table::Row row(table->columns());
      row.add_field(stat.campaign_id);
      row.add_field(CorbaAlgs::unpack_decimal<
        AdServer::CampaignSvcs::RevenueDecimal>(stat.amount).str());
      row.add_field(CorbaAlgs::unpack_decimal<
        AdServer::CampaignSvcs::RevenueDecimal>(stat.comm_amount).str());
      row.add_field(CorbaAlgs::unpack_decimal<
        AdServer::CampaignSvcs::RevenueDecimal>(stat.daily_amount).str());
      row.add_field(CorbaAlgs::unpack_decimal<
        AdServer::CampaignSvcs::RevenueDecimal>(stat.daily_comm_amount).str());
      table->add_row(row, filters_, sorter_);
    }

    table->dump(std::cout);
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady&)
  {
    std::cerr << FUN << ": caught NotReady" << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotSupport&)
  {
    std::cerr << FUN << ": caught NotSupport" << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void Application::stat(bool account)
  /*throw(Exception, InvalidArgument)*/
{
  static const char* FUN = "Application::stat()";

  init_server_ref_();
  try
  {
    AdServer::CampaignSvcs::StatInfo_var stat =
      server_->get_stat();

    if(account)
    {
      unsigned long columns = sizeof(STAT_AMOUNT_COLUMNS) /
        sizeof(STAT_AMOUNT_COLUMNS[0]);

      TablePtr table = TablePtr(new Table(columns));

      for(unsigned long j = 0; j < columns; ++j)
      {
        table->column(j, STAT_AMOUNT_COLUMNS[j]);
      }

      const AdServer::CampaignSvcs::AmountStatSeq& amounts =
        stat->accounts;

      for(CORBA::ULong i = 0; i < amounts.length(); ++i)
      {
        const AdServer::CampaignSvcs::AmountStatInfo& stat = amounts[i];

        Table::Row row(table->columns());
        row.add_field(stat.id);
        row.add_field(CorbaAlgs::unpack_decimal<
          AdServer::CampaignSvcs::RevenueDecimal>(stat.amount).str());
        row.add_field(CorbaAlgs::unpack_decimal<
          AdServer::CampaignSvcs::RevenueDecimal>(stat.comm_amount).str());
        row.add_field(CorbaAlgs::unpack_decimal<
          AdServer::CampaignSvcs::RevenueDecimal>(stat.daily_amount).str());
        row.add_field(CorbaAlgs::unpack_decimal<
          AdServer::CampaignSvcs::RevenueDecimal>(stat.daily_comm_amount).str());
        table->add_row(row, filters_, sorter_);
      }

      table->dump(std::cout);
    }
    else
    {
      unsigned long columns = sizeof(STAT_CCG_COLUMNS) /
        sizeof(STAT_CCG_COLUMNS[0]);

      TablePtr table = TablePtr(new Table(columns));

      for(unsigned long j = 0; j < columns; ++j)
      {
        table->column(j, STAT_CCG_COLUMNS[j]);
      }

      for(CORBA::ULong campaign_i = 0; campaign_i < stat->campaigns.length(); ++campaign_i)
      {
        const AdServer::CampaignSvcs::CampaignStatInfo& campaign_stat =
          stat->campaigns[campaign_i];

        for(CORBA::ULong i = 0; i < campaign_stat.ccgs.length(); ++i)
        {
          const AdServer::CampaignSvcs::CCGStatInfo& ccg_stat = campaign_stat.ccgs[i];

          Table::Row row(table->columns());
          row.add_field(ccg_stat.ccg_id);
          row.add_field(campaign_stat.campaign_id);
          row.add_field(CorbaAlgs::unpack_decimal<
            AdServer::CampaignSvcs::ImpRevenueDecimal>(ccg_stat.impressions).str());
          row.add_field(CorbaAlgs::unpack_decimal<
            AdServer::CampaignSvcs::ImpRevenueDecimal>(ccg_stat.clicks).str());
          row.add_field(CorbaAlgs::unpack_decimal<
            AdServer::CampaignSvcs::ImpRevenueDecimal>(ccg_stat.actions).str());
          row.add_field(CorbaAlgs::unpack_decimal<
            AdServer::CampaignSvcs::RevenueDecimal>(ccg_stat.amount).str());
          row.add_field(CorbaAlgs::unpack_decimal<
            AdServer::CampaignSvcs::RevenueDecimal>(ccg_stat.comm_amount).str());
          row.add_field(CorbaAlgs::unpack_decimal<
            AdServer::CampaignSvcs::RevenueDecimal>(ccg_stat.daily_amount).str());
          row.add_field(CorbaAlgs::unpack_decimal<
            AdServer::CampaignSvcs::RevenueDecimal>(ccg_stat.daily_comm_amount).str());

          {
            std::ostringstream hourly_amount_ostr;
            hourly_amount_ostr << "[ " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.current_hour_amount) << "] [" <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.prev_hour_amount) << "]";
            row.add_field(hourly_amount_ostr.str());
          }
        
          {
            std::ostringstream hourly_comm_amount_ostr;
            hourly_comm_amount_ostr << "[ " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.current_hour_comm_amount) << "] [" <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.prev_hour_comm_amount) << "]";
            row.add_field(hourly_comm_amount_ostr.str());
          }

          std::ostringstream creatives_ostr;
          for(CORBA::ULong cc_i = 0; cc_i < ccg_stat.creatives.length(); ++cc_i)
          {
            creatives_ostr << "[ " << ccg_stat.creatives[cc_i].cc_id << ": " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::ImpRevenueDecimal>(
                ccg_stat.creatives[cc_i].impressions).str() << ", " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::ImpRevenueDecimal>(
                ccg_stat.creatives[cc_i].clicks).str() << ", " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::ImpRevenueDecimal>(
                ccg_stat.creatives[cc_i].actions).str() << " ] ";
          }
          row.add_field(creatives_ostr.str());
          std::ostringstream publishers_ostr;
          for(CORBA::ULong pub_i = 0; pub_i < ccg_stat.publishers.length(); ++pub_i)
          {
            publishers_ostr << "[ " << ccg_stat.publishers[pub_i].pub_account_id <<
              ": " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.publishers[pub_i].amount) << " ] ";
          }
          row.add_field(publishers_ostr.str());
          std::ostringstream tags_ostr;
          for(CORBA::ULong tag_i = 0; tag_i < ccg_stat.tags.length(); ++tag_i)
          {
            tags_ostr << "[ " << ccg_stat.tags[tag_i].tag_id <<
              ": (" <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.tags[tag_i].cur_pub_isp_amount) << ", " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.tags[tag_i].cur_adv_amount) << ", " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.tags[tag_i].cur_adv_comm_amount) <<
              ") (" <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.tags[tag_i].prev_pub_isp_amount) << ", " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.tags[tag_i].prev_adv_amount) << ", " <<
              CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
                ccg_stat.tags[tag_i].prev_adv_comm_amount) << ") ] ";
          }
          row.add_field(tags_ostr.str());

          std::ostringstream ctr_resets_ostr;
          for(CORBA::ULong ctr_reset_i = 0;
              ctr_reset_i < ccg_stat.ctr_resets.length(); ++ctr_reset_i)
          {
            ctr_resets_ostr << "[ " << ccg_stat.ctr_resets[ctr_reset_i].ctr_reset_id <<
              ": " << CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::ImpRevenueDecimal>(
                ccg_stat.ctr_resets[ctr_reset_i].impressions).str() << " ] ";
          }
          row.add_field(ctr_resets_ostr.str());

          table->add_row(row, filters_, sorter_);
        }
      }

      table->dump(std::cout);
    }
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotReady&)
  {
    std::cerr << FUN << ": caught NotReady" << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotSupport&)
  {
    std::cerr << FUN << ": caught NotSupport" << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void Application::update_stat() /*throw(Exception)*/
{
  static const char* FUN = "Application::update_stat()";

  init_server_ref_();
  try
  {
    server_->update_stat();
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::NotSupport& ex)
  {
    std::cerr << FUN << ": caught NotSupport: " << ex.description << std::endl;
  }
  catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
  {
    std::cerr << FUN << ": caught ImplementationException: " << ex.description << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << FUN << ": caught CORBA::SystemException: " << ex << std::endl;
  }
}

void Application::describe_expression_channel(
  Table::Row& row,
  const AdServer::CampaignSvcs::ExpressionChannelBase* channel)
  /*throw(eh::Exception)*/
{
  const AdServer::CampaignSvcs::ChannelParams& ch_params = channel->params();

  row.add_field(ch_params.channel_id);
  row.add_field(ch_params.type);
  row.add_field(ch_params.country);
  row.add_field(ch_params.status);
  row.add_field(ch_params.action_id);
  row.add_field(ch_params.timestamp.get_gm_time().format(TIME_FORMAT));

  if(ch_params.descriptive_params.in())
  {
    row.add_field(ch_params.descriptive_params->name);
    row.add_field(ch_params.descriptive_params->parent_channel_id);
  }
  else
  {
    row.add_field(String::SubString());
    row.add_field(0);
  }

  if(ch_params.common_params.in())
  {
    row.add_field(ch_params.common_params->account_id);
    row.add_field(ch_params.common_params->is_public ? 1 : 0);
    row.add_field(ch_params.common_params->freq_cap_id);
    row.add_field(ch_params.common_params->language);
  }
  else
  {
    row.add_field(0);
    row.add_field(true);
    row.add_field(0);
    row.add_field(String::SubString());
  }

  std::ostringstream ostr;
  AdServer::CampaignSvcs::print(ostr, channel);
  row.add_field(ostr.str());

  row.add_field(ch_params.discover_params.in() ?
    ch_params.discover_params->query : std::string());
  row.add_field(ch_params.discover_params.in() ?
    ch_params.discover_params->annotation : std::string());
  row.add_field(ch_params.discover_params.in() ?
    ch_params.discover_params->create_time.get_gm_time().format(TIME_FORMAT) :
    std::string());

  row.add_field(ch_params.cmp_params.in() ?
    ch_params.cmp_params->channel_rate_id : 0);
  row.add_field(ch_params.cmp_params.in() ?
    ch_params.cmp_params->imp_revenue.str() : std::string());
  row.add_field(ch_params.cmp_params.in() ?
    ch_params.cmp_params->click_revenue.str() : std::string());
}
