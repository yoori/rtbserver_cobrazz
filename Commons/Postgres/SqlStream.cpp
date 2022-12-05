#include<Commons/Postgres/SqlStream.hpp> 
#include<String/AsciiStringManip.hpp>
#include<iostream>

namespace
{
  String::AsciiStringManip::Char2Category<'\\', '"'> EscapedCategory;
}

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    void SqlStream::set_date(const Generics::Time& date) /*throw(eh::Exception)*/
    {
      next_field_();
      stream_ << date.get_gm_time().format("%Y-%m-%d");
    }

    void SqlStream::set_time(const Generics::Time& time) /*throw(eh::Exception)*/
    {
      next_field_();
      stream_ << time.get_gm_time().format("%H:%M:%S.%q");
    }

    void SqlStream::set_timestamp(const Generics::Time& timestamp)
      /*throw(eh::Exception)*/
    {
      next_field_();
      stream_ << timestamp.get_gm_time().format("%Y-%m-%d %H:%M:%S.%q");
    }

    void SqlStream::set_string(const std::string& str)
      /*throw(eh::Exception)*/
    {
      next_field_();
      stream_ << "\\\"";
      //escaping of string if necessary
      const char* c_str = str.data();
      const char* c_str_end = c_str + str.size();
      const char* esc_sym = EscapedCategory.find_owned(c_str, c_str_end); 
      if(esc_sym != c_str_end)
      {
        do
        {
          std::cerr << c_str - str.data() << " " << esc_sym - str.data() << " " << c_str_end - str.data() << std::endl;
          if(c_str != esc_sym)
          {
            stream_ << String::SubString(c_str, esc_sym);
          }
          stream_ << '\\' << *esc_sym;
          c_str = esc_sym + 1;
          esc_sym = EscapedCategory.find_owned(c_str, c_str_end); 
        } while(esc_sym < c_str_end);
        if (c_str < c_str_end)
        {
          stream_ << String::SubString(c_str, c_str_end);
        }
      }
      else
      {
        stream_ << str;
      }
      stream_ << "\\\""; 
    }

    void SqlStream::set_null() /*throw(eh::Exception)*/
    {
      set_value("NULL");
    }

    void SqlStream::next_field_() noexcept
    {
      if (pos_ != 0)
      {
        stream_ << ',';
      }
      pos_++;
    }

  }
}
}

