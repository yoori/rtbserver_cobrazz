// $Id: ProcessControlVarsImpl.cpp 185978 2020-07-04 00:12:49Z jurij_kuznecov $

#include "ProcessControlVarsImpl.hpp"

namespace AdServer {
namespace Commons {

const char LogLevelProcessor::VAR_NAME[] = "LOG_LEVEL";
const char DbStateProcessor::VAR_NAME[] = "DB";

char*
ProcessControlVarsImpl::control(const char *var_name, const char *var_value)
  /*throw(Exception)*/
{
  static const char* FUN = "ProcessControlVarsImpl::control()";

  VarProcessorMapT::const_iterator it = var_proc_map_.find(var_name);
  if (it == var_proc_map_.end())
  {
    Stream::Error err;
    err << "No processor found for variable: '" << var_name << "'";
    throw Exception(err);
  }

  std::string result;

  try
  {
    if (!it->second->set_value(var_value, result))
    {
      result = "Failed to set variable '";
      result += var_name;
      result +="' to '";
      result += var_value;
      result += "'";
    }
  }
  catch(const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << FUN << ": caught eh::Exception: " << ex.what();
    throw Exception(ostr);
  }

  return CORBA::string_dup(result.c_str());
}

} // namespace Commons
} // namespace AdServer
