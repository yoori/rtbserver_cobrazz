#ifndef USERTRIGGERMATCHPROFILEUTILS_HPP
#define USERTRIGGERMATCHPROFILEUTILS_HPP

#include <Generics/Time.hpp>
#include <UtilCommons/Table.hpp>
#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestTriggerMatchProfile.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  void print_user_trigger_match_profile(
    std::ostream& out,
    const char* user_id,
    const UserTriggerMatchReader& reader,
    bool align = false)
    noexcept;
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    const Table::Column TRIGGER_MATCH_PROFILE_TABLE_COLUMNS[] =
    {
      Table::Column("user_id", Table::Column::TEXT),
      Table::Column("page_matches", Table::Column::TEXT),
      Table::Column("search_matches", Table::Column::TEXT),
      Table::Column("url_matches", Table::Column::TEXT),
      Table::Column("url_keyword_matches", Table::Column::TEXT),
      Table::Column("requests", Table::Column::TEXT),
    };
  }

  template<typename IteratorType>
  std::string
  print_match_list(
    const IteratorType& begin, const IteratorType& end, const char* offset, bool align)
  {
    std::ostringstream ostr;
    for(IteratorType it = begin; it != end; ++it)
    {
      if (align)
      {
        if (it != begin)
        {
          ostr << ", ";
        }
      }
      else
      {
        ostr << std::endl << offset;
      }
      ostr << (*it).channel_id() << " : " <<
        Generics::Time((*it).time()).gm_ft() << " [";
      Algs::print(ostr, (*it).positive_matches().begin(), (*it).positive_matches().end());
      ostr << "] [";
      Algs::print(ostr, (*it).negative_matches().begin(), (*it).negative_matches().end());
      ostr << "]";
    }
    return ostr.str();
  }

  inline
  void print_user_trigger_match_profile(
    std::ostream& out,
    const char* user_id,
    const UserTriggerMatchReader& profile_reader,
    bool align)
    noexcept
  {
    unsigned long columns =
      sizeof(TRIGGER_MATCH_PROFILE_TABLE_COLUMNS) /
      sizeof(TRIGGER_MATCH_PROFILE_TABLE_COLUMNS[0]);

    Table table(columns);
      
    for(unsigned long i = 0; i < columns; i++)
    {
      table.column(i, TRIGGER_MATCH_PROFILE_TABLE_COLUMNS[i]);
    }

    std::string::size_type max_len = 0;
    for (unsigned long ind = 0; ind < table.columns(); ++ind)
    {
      max_len = std::max(
        max_len,
        TRIGGER_MATCH_PROFILE_TABLE_COLUMNS[ind].name.length());
    }

    std::string prefix(max_len + 3, ' ');

    Table::Row row(table.columns());
      
    row.add_field(user_id);
    row.add_field(print_match_list(
      profile_reader.page_matches().begin(),
      profile_reader.page_matches().end(),
      prefix.c_str(),
      align));
    row.add_field(print_match_list(
      profile_reader.search_matches().begin(),
      profile_reader.search_matches().end(),
      prefix.c_str(),
      align));
    row.add_field(print_match_list(
      profile_reader.url_matches().begin(),
      profile_reader.url_matches().end(),
      prefix.c_str(),
      align));
    row.add_field(print_match_list(
      profile_reader.url_keyword_matches().begin(),
      profile_reader.url_keyword_matches().end(),
      prefix.c_str(),
      align));
    
    {
      std::ostringstream ostr;
      for(UserTriggerMatchReader::impressions_Container::const_iterator imp_it =
            profile_reader.impressions().begin();
          imp_it != profile_reader.impressions().end(); ++imp_it)
      {
        ostr << std::endl << prefix << "[ time = " <<
          Generics::Time((*imp_it).time()).gm_ft() <<
          ", request_id = " << (*imp_it).request_id() << " ]";
      }
      row.add_field(ostr.str());
    }

    table.add_row(row);
    table.dump(out);
    out << std::endl;
  }
}
}

#endif /*USERCHANNELINVENTORYPROFILEUTILS_HPP*/
