#include <string>

#include <String/StringManip.hpp>
#include <String/UTF8Case.hpp>
#include <String/Tokenizer.hpp>

#include "Table.hpp"

namespace
{
  String::AsciiStringManip::Category::CharTable rel_tavle("<>=!~");
}

void
Table::column(unsigned long i, const Column& column)
  /*throw(OutOfRange, Exception, eh::Exception)*/
{
  if(i >= columns())
  {
    Stream::Error ostr;
    ostr << "Table::header: failed to set '" << column.name
         << "' header name as " << i << " is out of [0," << columns()
         << ") range";

    throw OutOfRange(ostr);
  }

  columns_[i] = column;
}

void
Table::add_row(const Row& row, const Filters& filters, const Sorter& sorter)
  /*throw(InvalidArgument, eh::Exception)*/
{
  if(row.size() != columns())
  {
    Stream::Error ostr;
    ostr << "Table::add_row: row contain " << row.size()
         << " fields instead of " << columns();

    throw InvalidArgument(ostr);
  }

  for(unsigned long i = 0; i < columns(); i++)
  {
    for(Filters::const_iterator it = filters.begin();
        it != filters.end(); ++it)
    {
      if(!strcasecmp(columns_[i].name.c_str(), it->column_name.c_str())
         && !it->fulfill(row[i], columns_[i].type))
      {
        return;
      }
    }
  }

  for(unsigned long i = 0; i < columns(); i++)
  {
    if(columns_[i].name == sorter.column_name)
    {
      for(Rows::iterator it = rows_.begin(); it != rows_.end(); ++it)
      {
        if(sorter.insert_before(row[i], (*it)[i], columns_[i].type))
        {
          rows_.insert(it, row);
          return;
        }
      }
    }
  }

  rows_.push_back(row);
}

unsigned long
Table::value_align() const noexcept
{
  unsigned long max_len = 0;

  for(unsigned long i = 0; i < columns(); i++)
  {
    if(max_len < columns_[i].name.length())
    {
      max_len = columns_[i].name.length();
    }
  }

  return max_len;
}

void
Table::dump(std::ostream& ostr, const char* prefix) const /*throw(Exception, eh::Exception)*/
{
  unsigned long max_len = value_align();

  for(Rows::const_iterator it = rows_.begin(); it != rows_.end(); ++it)
  {
    for(unsigned long i = 0; i < columns(); i++)
    {
      if(prefix)
      {
        ostr << prefix;
      }

      ostr.width(max_len);
      ostr.fill(' ');

      ostr << columns_[i].name << " : " << (*it)[i] << std::endl;
    }

    ostr << std::endl;
  }
}

//
// Table::FilterPrivate class
//

bool
Table::FilterPrivate::fulfill(const String::SubString& field, const char* typenam, ColumnOperationHandler* handler) const
  /*throw(eh::Exception)*/
{
  std::string text_value;
  String::case_change<String::Uniform>(value, text_value);

  try
  {
    handler->init(field, text_value);
    bool ret = (*handler)(relation);
    return ret;
  }
  catch (const ColumnOperationHandler::Exception &ex)
  {
    Stream::Error ostr;
    ostr << "Table::Filter::fulfill(): Column type = "
      << typenam << ". Caught "
      << "ColumnOperationHandler::Exception. "
      << ": " << ex.what();
    throw Exception(ostr);
  }
}

//
// Table::Filter class
//

Table::Filter::Filter(const char* nm, const char* vl, Relation rel)
  /*throw(eh::Exception)*/
  : column_name(nm ? nm : ""),
    relation(rel)
{
  column_type_names_[Column::TEXT] = "Column::TEXT";
  column_type_names_[Column::NUMBER] = "Column::NUMBER";
  column_type_names_[Column::REAL] = "Column::REAL";
  static TextOperationHandler text_op_handler;
  static NumberOperationHandler number_op_handler;
  static RealOperationHandler real_op_handler;
  column_type_handlers_[Column::TEXT] = &text_op_handler;
  column_type_handlers_[Column::NUMBER] = &number_op_handler;
  column_type_handlers_[Column::REAL] = &real_op_handler;
  //
  String::SubString s(vl);
  String::StringManip::SplitComma tokenizer(s);
  String::SubString value;
  while (tokenizer.get_token(value))
  {
    filters_.push_back(FilterPrivate(value.str().c_str(), rel));
  }
}

bool
Table::Filter::fulfill(const String::SubString& field, Column::Type type) const
  /*throw(eh::Exception)*/
{
  ColumnTypeHandlerMap::const_iterator handler_it =
    column_type_handlers_.find(type);
  ColumnTypeNameMap::const_iterator name_it =
    column_type_names_.find(type);

  if (handler_it == column_type_handlers_.end() || name_it == column_type_names_.end())
  {
    return false;
  }

  std::string text_field;
  String::case_change<String::Uniform>(field, text_field);

  if(relation < RL_NE)
  {
    bool ret = false;
    for(Filters::const_iterator it = filters_.begin(); it != filters_.end(); ++it)
    {
      ret = ret || it->fulfill(text_field, name_it->second, handler_it->second);
    }
    return ret;
  }
  else
  {
    bool ret = true;
    for(Filters::const_iterator it = filters_.begin(); it != filters_.end(); ++it)
    {
      ret = ret && it->fulfill(text_field, name_it->second, handler_it->second);
    }
    return ret;
  }
}

//
// Table::Sorter class
//

bool
Table::Sorter::insert_before(const String::SubString& new_field,
                             const String::SubString& existing_field,
                             Column::Type field_type) const
  /*throw(eh::Exception)*/
{
  std::string text_new_field;
  std::string text_existing_field;

  long long num_new_field = 0;
  long long num_existing_field = 0;

  double real_new_field = 0.;
  double real_existing_field = 0.;

  if(field_type == Column::NUMBER)
  {
    Stream::Parser istr_new_field(new_field);
    istr_new_field >> num_new_field;

    Stream::Parser istr_existing_field(existing_field);
    istr_existing_field >> num_existing_field;
  }
  else if (field_type == Column::REAL)
  {
    Stream::Parser istr_new_field(new_field);
    istr_new_field >> real_new_field;

    Stream::Parser istr_existing_field(existing_field);
    istr_existing_field >> real_existing_field;
  }
  else
  {
    String::case_change<String::Uniform>(new_field, text_new_field);
    String::case_change<String::Uniform>(existing_field, text_existing_field);
  }

  if(descending)
  {
    if(field_type == Column::NUMBER)
    {
      return num_new_field >= num_existing_field;
    }
    else if (field_type == Column::REAL)
    {
      return real_new_field >= real_existing_field;
    }
    else
    {
      return strcmp(text_new_field.c_str(),
                            text_existing_field.c_str()) >= 0;
    }
  }
  else
  {
    if(field_type == Column::NUMBER)
    {
      return num_new_field <= num_existing_field;
    }
    else if (field_type == Column::REAL)
    {
      return real_new_field <= real_existing_field;
    }
    else
    {
      return strcmp(text_new_field.c_str(),
                            text_existing_field.c_str()) <= 0;
    }
  }
}

bool Table::parse_filter(
  const char* c_str,
  std::string& name,
  Table::Relation& relation,
  std::string& value) noexcept
{
  const char* it =
    String::AsciiStringManip::Category::Category<
    String::AsciiStringManip::Category::CharTable>
    (rel_tavle).find_owned(c_str);

  if (it)
  {
    const char* v_it = 0; 
    relation = RL_EQ;
    switch(*it)
    {
      case '!':
        if (*(it + 1) == '=')
        {
          relation = Table::RL_NE;
        }
        else if (*(it + 1) == '~')
        { 
          relation = Table::RL_NC;
        }
        else
        {
          return false;
        }
        v_it = it + 2;
        break;
      case '<':
        if (*(it + 1) == '=')
        {
          relation = Table::RL_NG;
          v_it = it + 2;
        }
        else
        {
          relation = Table::RL_LT;                                                                       
          v_it = it + 1;
        }
        break;
      case '>':
        if (*(it + 1) == '=')
        {
          relation = Table::RL_NL;
          v_it = it + 2;
        }
        else
        {
          relation = Table::RL_GT;                                                                       
          v_it = it + 1;
        }
        break;
      case '=':
        relation = Table::RL_EQ;                                               
        v_it = it + 1;
        break;
      case '~':
        relation = Table::RL_CN;                                               
        v_it = it + 1;
        break;
    }
    name.assign(c_str, it);
    value = v_it;
    return true;
  }
  return false;
}


