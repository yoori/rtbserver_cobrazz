
#include "FailContext.hpp"
#include <Commons/PathManip.hpp>

namespace AutoTest
{
  void fail_message(
    Stream::Error& ostr,
    const String::SubString& description,
    const char* file_path,
    const char* function_name,
    const unsigned int line,
    const char* notes)
    noexcept
  {
    std::string file_name;
    AdServer::PathManip::split_path(file_path, 0, &file_name);

    ostr << description << std::endl <<
      "  " << file_name << ":" << line << ": " << function_name << "()";
    if(notes[0])
    {
      ostr << ": " << notes;
    }
  }
  
  void fail(
    const String::SubString& description,
    const char* file_path,
    const char* function_name,
    const unsigned int line,
    const char* notes)
    /*throw(Exception)*/
  {
    Stream::Error ostr;
    fail_message(ostr, description, file_path, function_name, line, notes);
    throw Exception(ostr);
  }

}

