
#include <sstream>
#include <fstream>

#include "UserAgentMatcher.hpp"
#include <Stream/MemoryStream.hpp>

namespace FrontendCommons
{
  void UserAgentMatcher::init(const char *filename)
    /*throw(Exception, eh::Exception)*/
  {
    hash_set_.clear();
    std::ifstream ifs(filename);
    if (!ifs)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__ << ": failed to open file "
        << (filename ? filename : "<filename is NULL>");
      throw Exception(ostr);
    }
    std::string user_agent;
    while (std::getline(ifs, user_agent))
    {
      hash_set_.insert(user_agent);
    }
  }
} // namespace FrontendCommons

