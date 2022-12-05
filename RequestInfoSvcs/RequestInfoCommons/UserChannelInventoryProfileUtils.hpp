#ifndef USERCHANNELINVENTORYPROFILEUTILS_HPP
#define USERCHANNELINVENTORYPROFILEUTILS_HPP

#include <Generics/Time.hpp>
#include <UtilCommons/Table.hpp>
#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  void print_channel_inventory_day_profile(
    std::ostream& out,
    const ChannelInventoryDayReader& inv_day_profile_reader)
    noexcept;

  void
  print_user_colo_reach_profile(
    std::ostream& out,
    const char* user_id,
    const UserColoReachProfileReader& profile_reader)
    noexcept;

  void print_user_channel_inventory_profile(
    std::ostream& out,
    const char* user_id,
    const UserChannelInventoryProfileReader& reader)
    noexcept;
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    const Table::Column COLO_REACH_PROFILE_TABLE_COLUMNS[] =
    {
      Table::Column("user_id", Table::Column::TEXT),

      Table::Column("create_time", Table::Column::TEXT),
      Table::Column("isp_create_time", Table::Column::TEXT),
      Table::Column("colo_appearance_list", Table::Column::TEXT),
      Table::Column("isp_colo_appearance_list", Table::Column::TEXT),
    };

    const Table::Column CHANNEL_INVENTORY_PROFILE_TABLE_COLUMNS[] =
    {
      Table::Column("user_id", Table::Column::TEXT),

      Table::Column("create_time", Table::Column::TEXT),
      Table::Column("isp_create_time", Table::Column::TEXT),
      Table::Column("last_request_time", Table::Column::TEXT),
      Table::Column("last_daily_processing_time", Table::Column::TEXT),
      Table::Column("sum_revenue", Table::Column::TEXT),
      Table::Column("imp_count", Table::Column::NUMBER),
      Table::Column("colo_appearance_list", Table::Column::TEXT),
      Table::Column("colo_ad_appearance_list", Table::Column::TEXT),
      Table::Column("colo_merge_appearance_list", Table::Column::TEXT),
      Table::Column("isp_colo_appearance_list", Table::Column::TEXT),
      Table::Column("isp_colo_ad_appearance_list", Table::Column::TEXT),
      Table::Column("isp_colo_merge_appearance_list", Table::Column::TEXT)
    };

    const Table::Column CHANNEL_INVENTORY_DAY_PROFILE_TABLE_COLUMNS[] =
    {
      Table::Column("day", Table::Column::TEXT),

      Table::Column("total_channels", Table::Column::TEXT),
      Table::Column("active_channels", Table::Column::TEXT),

      Table::Column("display_imp_other_channels", Table::Column::TEXT),
      Table::Column("display_imp_channels", Table::Column::TEXT),
      Table::Column("display_impop_no_imp_channels", Table::Column::TEXT),

      Table::Column("text_imp_other_channels", Table::Column::TEXT),
      Table::Column("text_imp_channels", Table::Column::TEXT),
      Table::Column("text_impop_no_imp_channels", Table::Column::TEXT),

      Table::Column("channel_ecpms", Table::Column::TEXT)
    };
  }

  template<typename IteratorType>
  std::string
  print_channel_list(const IteratorType& begin, const IteratorType& end)
  {
    std::ostringstream ostr;
    Algs::print(ostr, begin, end);
    return ostr.str();
  }

  inline
  void print_channel_inventory_day_profile(
    std::ostream& out,
    const ChannelInventoryDayReader& inv_day_profile_reader)
    noexcept
  {
    unsigned long columns =
      sizeof(CHANNEL_INVENTORY_DAY_PROFILE_TABLE_COLUMNS) /
      sizeof(CHANNEL_INVENTORY_DAY_PROFILE_TABLE_COLUMNS[0]);
      
    Table table(columns);
      
    for(unsigned long i = 0; i < columns; i++)
    {
      table.column(i, CHANNEL_INVENTORY_DAY_PROFILE_TABLE_COLUMNS[i]);
    }

    std::string::size_type max_len = 0;
    for (unsigned long ind = 0; ind < table.columns(); ++ind)
    {
      std::string::size_type len =
        CHANNEL_INVENTORY_DAY_PROFILE_TABLE_COLUMNS[ind].name.length();
      max_len = std::max(len, max_len);
    }

    std::string prefix(max_len + 3, ' ');

    Table::Row row(table.columns());
      
    row.add_field(Generics::Time(inv_day_profile_reader.date()).gm_ft());

    row.add_field(print_channel_list(
      inv_day_profile_reader.total_channel_list().begin(),
      inv_day_profile_reader.total_channel_list().end()));
    row.add_field(print_channel_list(
      inv_day_profile_reader.active_channel_list().begin(),
      inv_day_profile_reader.active_channel_list().end()));

    row.add_field(print_channel_list(
      inv_day_profile_reader.display_imp_other_channel_list().begin(),
      inv_day_profile_reader.display_imp_other_channel_list().end()));
    row.add_field(print_channel_list(
      inv_day_profile_reader.display_imp_channel_list().begin(),
      inv_day_profile_reader.display_imp_channel_list().end()));
    row.add_field(print_channel_list(
      inv_day_profile_reader.display_impop_no_imp_channel_list().begin(),
      inv_day_profile_reader.display_impop_no_imp_channel_list().end()));
      
    row.add_field(print_channel_list(
      inv_day_profile_reader.text_imp_other_channel_list().begin(),
      inv_day_profile_reader.text_imp_other_channel_list().end()));
    row.add_field(print_channel_list(
      inv_day_profile_reader.text_imp_channel_list().begin(),
      inv_day_profile_reader.text_imp_channel_list().end()));
    row.add_field(print_channel_list(
      inv_day_profile_reader.text_impop_no_imp_channel_list().begin(),
      inv_day_profile_reader.text_impop_no_imp_channel_list().end()));

    {
      std::ostringstream ostr;
      
      for(ChannelInventoryDayReader::channel_price_ranges_Container::const_iterator it =
            inv_day_profile_reader.channel_price_ranges().begin();
          it != inv_day_profile_reader.channel_price_ranges().end();
          ++it)
      {
        if(it != inv_day_profile_reader.channel_price_ranges().begin())
        {
          ostr << prefix;
        }
          
        ostr << "[" << (*it).country() << ", " <<
          (*it).tag_size() << "]: "
          "ecpm = " << (*it).ecpm() << ", {";

        Algs::print(ostr,
          (*it).channel_list().begin(), (*it).channel_list().end());

        ostr << "}" << std::endl;
      }

      row.add_field(ostr.str());
    }
      
    table.add_row(row);
    table.dump(out);
  }

  inline
  void print_user_colo_reach_profile(
    std::ostream& out,
    const char* user_id,
    const UserColoReachProfileReader& profile_reader)
    noexcept
  {
    unsigned long columns =
      sizeof(COLO_REACH_PROFILE_TABLE_COLUMNS) /
      sizeof(COLO_REACH_PROFILE_TABLE_COLUMNS[0]);
      
    Table table(columns);
      
    for(unsigned long i = 0; i < columns; i++)
    {
      table.column(i, COLO_REACH_PROFILE_TABLE_COLUMNS[i]);
    }

    Table::Row row(table.columns());
    std::string space_align(table.value_align(), ' ');

    row.add_field(user_id);

    row.add_field(
      Generics::Time(profile_reader.create_time()).gm_ft());
    row.add_field(
      AdServer::RequestInfoSvcs::stringify_id_date_list(
        profile_reader.isp_colo_create_time(), space_align.c_str()));

    row.add_field(AdServer::RequestInfoSvcs::stringify_appearance_list(
      profile_reader.colo_appears(), space_align.c_str()));

    row.add_field(AdServer::RequestInfoSvcs::stringify_appearance_list(
      profile_reader.isp_colo_appears(), space_align.c_str()));

    table.add_row(row);
    table.dump(out);
    out << std::endl;
  }
  
  inline
  void print_user_channel_inventory_profile(
    std::ostream& out,
    const char* user_id,
    const UserChannelInventoryProfileReader& inv_profile_reader)
    noexcept
  {
    unsigned long columns =
      sizeof(CHANNEL_INVENTORY_PROFILE_TABLE_COLUMNS) /
      sizeof(CHANNEL_INVENTORY_PROFILE_TABLE_COLUMNS[0]);
      
    Table table(columns);
      
    for(unsigned long i = 0; i < columns; i++)
    {
      table.column(i, CHANNEL_INVENTORY_PROFILE_TABLE_COLUMNS[i]);
    }

    Table::Row row(table.columns());
    std::string space_align(table.value_align(), ' ');

    row.add_field(user_id);

    row.add_field(
      Generics::Time(inv_profile_reader.create_time()).gm_ft());
    row.add_field(
      AdServer::RequestInfoSvcs::stringify_id_date_list(
        inv_profile_reader.isp_colo_create_time(), space_align.c_str()));
    row.add_field(
      Generics::Time(inv_profile_reader.last_request_time()).gm_ft());
    row.add_field(
      Generics::Time(inv_profile_reader.last_daily_processing_time()).
        gm_ft());

    row.add_field(inv_profile_reader.sum_revenue());
    row.add_field(inv_profile_reader.imp_count());

    row.add_field(AdServer::RequestInfoSvcs::stringify_appearance_list(
      inv_profile_reader.colo_appears(), space_align.c_str()));
    row.add_field(AdServer::RequestInfoSvcs::stringify_appearance_list(
      inv_profile_reader.colo_ad_appears(), space_align.c_str()));
    row.add_field(AdServer::RequestInfoSvcs::stringify_appearance_list(
      inv_profile_reader.colo_merge_appears(), space_align.c_str()));

    row.add_field(AdServer::RequestInfoSvcs::stringify_appearance_list(
      inv_profile_reader.isp_colo_appears(), space_align.c_str()));
    row.add_field(AdServer::RequestInfoSvcs::stringify_appearance_list(
      inv_profile_reader.isp_colo_ad_appears(), space_align.c_str()));
    row.add_field(AdServer::RequestInfoSvcs::stringify_appearance_list(
      inv_profile_reader.isp_colo_merge_appears(), space_align.c_str()));

    table.add_row(row);
    table.dump(out);
    out << std::endl;

    for(UserChannelInventoryProfileReader::days_Container::const_iterator it =
            inv_profile_reader.days().begin();
        it != inv_profile_reader.days().end(); ++it)
    {
      print_channel_inventory_day_profile(out, *it);
      out << std::endl;
    }
  }
}
}

#endif /*USERCHANNELINVENTORYPROFILEUTILS_HPP*/
