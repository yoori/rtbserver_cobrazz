#ifndef _USERACTIONPROFILEUTILS_HPP_
#define _USERACTIONPROFILEUTILS_HPP_

#include <sstream>
#include <Generics/Time.hpp>
#include <UtilCommons/Table.hpp>
#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserActionProfile.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  void
  print_user_action_profile(
    std::ostream& out,
    const UserActionProfileReader& reader)
    noexcept;
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    const Table::Column USER_ACTION_TABLE_COLUMNS[] =
    {
      Table::Column("action_markers", Table::Column::TEXT),
      Table::Column("wait_markers", Table::Column::TEXT),
      Table::Column("custom_action_markers", Table::Column::TEXT),
      Table::Column("custom_wait_actions", Table::Column::TEXT),
      Table::Column("custom_done_actions", Table::Column::TEXT)
    };
  }

  void
  print_user_action_profile(
    std::ostream& out,
    const UserActionProfileReader& user_action_reader,
    bool align)
    noexcept
  {
    unsigned long columns =
      sizeof(USER_ACTION_TABLE_COLUMNS) /
      sizeof(USER_ACTION_TABLE_COLUMNS[0]);

    Table table(columns);

    for(unsigned long i = 0; i < columns; i++)
    {
      table.column(i, USER_ACTION_TABLE_COLUMNS[i]);
    }

    Table::Row row(table.columns());

    std::string prefix;
    std::string records_separator;

    if(align)
    {
      records_separator = " ";
    }
    else
    {
      prefix = std::string("\n") + std::string(table.value_align() + 3, ' ');
      records_separator = prefix;
    }
    
    std::ostringstream action_markers_str;
  
    for(UserActionProfileReader::action_markers_Container::const_iterator it =
          user_action_reader.action_markers().begin();
        it != user_action_reader.action_markers().end(); ++it)
    {
      action_markers_str
        << (it == user_action_reader.action_markers().begin() ?
            prefix.c_str() : records_separator.c_str())
        << "[ ccg_id = " << (*it).ccg_id()
        << ", cc_id = " << (*it).cc_id()
        << ", request_id = " << (*it).request_id()
        << ", time = " << Generics::Time((*it).time()).gm_ft() << " ]";
    }

    std::ostringstream wait_actions_str;

    for(UserActionProfileReader::wait_actions_Container::const_iterator it =
          user_action_reader.wait_actions().begin();
        it != user_action_reader.wait_actions().end(); ++it)
    {
      wait_actions_str <<
        (it == user_action_reader.wait_actions().begin() ?
         prefix.c_str() : records_separator.c_str()) <<
        "[ ccg_id = " << (*it).ccg_id() <<
        ", time = " << Generics::Time((*it).time()).gm_ft() <<
        ", count = " << (*it).count() << " ]";
    }

    std::ostringstream custom_action_markers_str;
    std::ostringstream custom_wait_actions_str;
    std::ostringstream custom_done_actions_str;

    for(UserActionProfileReader::custom_action_markers_Container::const_iterator it =
          user_action_reader.custom_action_markers().begin();
        it != user_action_reader.custom_action_markers().end(); ++it)
    {
      custom_action_markers_str <<
        (it == user_action_reader.custom_action_markers().begin() ?
         prefix.c_str() : records_separator.c_str()) <<
        "[ action_id = " << (*it).action_id() <<
        ", action_request_id = " << (*it).action_request_id() <<
        ", referer = '" << (*it).referer() << "'"
        ", order_id = " << (*it).order_id() <<
        ", action_value = " << (*it).action_value() <<
        ", time = " << Generics::Time((*it).time()).gm_ft() <<
        ", ccg_ids = ";
      Algs::print(custom_action_markers_str,
       (*it).ccg_ids().begin(), (*it).ccg_ids().end(), ",");
      custom_action_markers_str << " ]";
    }

    for(UserActionProfileReader::done_impressions_Container::const_iterator
          it = user_action_reader.done_impressions().begin();
        it != user_action_reader.done_impressions().end(); ++it)
    {
      custom_wait_actions_str <<
        (it == user_action_reader.done_impressions().begin() ?
         prefix.c_str() : records_separator.c_str()) <<
        "[ ccg_id = " << (*it).ccg_id() <<
        ", time = " << Generics::Time((*it).time()).gm_ft() <<
        ", request_id = " << (*it).request_id() << " ]";
    }

    for(UserActionProfileReader::custom_done_actions_Container::const_iterator
          it = user_action_reader.custom_done_actions().begin();
        it != user_action_reader.custom_done_actions().end(); ++it)
    {
      custom_done_actions_str <<
        (it == user_action_reader.custom_done_actions().begin() ?
         prefix.c_str() : records_separator.c_str()) <<
        "[ action_id = " << (*it).action_id() <<
        ", time = " << Generics::Time((*it).time()).gm_ft() << " ]";
    }

    row.add_field(action_markers_str.str());
    row.add_field(wait_actions_str.str());
    row.add_field(custom_action_markers_str.str());
    row.add_field(custom_wait_actions_str.str());
    row.add_field(custom_done_actions_str.str());

    table.add_row(row);
    table.dump(out);
  }
}
}

#endif /*_USERACTIONPROFILEUTILS_HPP_*/
