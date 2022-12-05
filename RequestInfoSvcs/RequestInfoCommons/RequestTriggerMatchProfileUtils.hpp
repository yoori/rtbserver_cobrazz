#ifndef REQUESTTRIGGERMATCHPROFILEUTILS_HPP
#define REQUESTTRIGGERMATCHPROFILEUTILS_HPP

#include <Generics/Time.hpp>
#include <UtilCommons/Table.hpp>
#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestTriggerMatchProfile.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  void
  print_request_trigger_match_profile(
    std::ostream& out,
    const char* request_id,
    const RequestTriggerMatchReader& reader)
    noexcept;
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    const Table::Column REQUEST_TRIGGER_MATCH_PROFILE_TABLE_COLUMNS[] =
    {
      Table::Column("request_id", Table::Column::TEXT),
      Table::Column("time", Table::Column::TEXT),
      Table::Column("page_matches", Table::Column::TEXT),
      Table::Column("search_matches", Table::Column::TEXT),
      Table::Column("url_matches", Table::Column::TEXT),
      Table::Column("url_keyword_matches", Table::Column::TEXT),
      Table::Column("click_done", Table::Column::NUMBER),
    };

    template<typename IteratorType>
    std::string
    print_match_count_list(
      const IteratorType& begin, const IteratorType& end, const char* offset)
    {
      std::ostringstream ostr;
      ostr << std::endl;
      for(IteratorType it = begin; it != end; ++it)
      {
        ostr << offset << "[" << (*it).channel_trigger_id() << " : " <<
          (*it).counter() << "]" << std::endl;
      }
      return ostr.str();
    }
  }

  inline void
  print_request_trigger_match_profile(
    std::ostream& out,
    const char* request_id,
    const RequestTriggerMatchReader& profile_reader)
    noexcept
  {
    unsigned long columns =
      sizeof(REQUEST_TRIGGER_MATCH_PROFILE_TABLE_COLUMNS) /
      sizeof(REQUEST_TRIGGER_MATCH_PROFILE_TABLE_COLUMNS[0]);

    Table table(columns);
      
    for(unsigned long i = 0; i < columns; i++)
    {
      table.column(i, REQUEST_TRIGGER_MATCH_PROFILE_TABLE_COLUMNS[i]);
    }

    std::string::size_type max_len = 0;
    for (unsigned long ind = 0; ind < table.columns(); ++ind)
    {
      max_len = std::max(
        max_len,
        REQUEST_TRIGGER_MATCH_PROFILE_TABLE_COLUMNS[ind].name.length());
    }

    std::string prefix(max_len + 3, ' ');

    Table::Row row(table.columns());

    row.add_field(request_id);
    row.add_field(Generics::Time(profile_reader.time()).gm_ft());
    row.add_field(print_match_count_list(
      profile_reader.page_match_counters().begin(),
      profile_reader.page_match_counters().end(),
      prefix.c_str()));
    row.add_field(print_match_count_list(
      profile_reader.search_match_counters().begin(),
      profile_reader.search_match_counters().end(),
      prefix.c_str()));
    row.add_field(print_match_count_list(
      profile_reader.url_match_counters().begin(),
      profile_reader.url_match_counters().end(),
      prefix.c_str()));
    row.add_field(print_match_count_list(
      profile_reader.url_keyword_match_counters().begin(),
      profile_reader.url_keyword_match_counters().end(),
      prefix.c_str()));
    row.add_field(profile_reader.click_done());

    table.add_row(row);
    table.dump(out);
    out << std::endl;
  }
}
}

#endif /*REQUESTTRIGGERMATCHPROFILEUTILS_HPP*/
