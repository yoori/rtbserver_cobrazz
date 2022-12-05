#ifndef USERIDBLACKLIST_HPP__
#define USERIDBLACKLIST_HPP__

#include <string>
//#include <istream>
#include <sstream>

#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/Function.hpp>
#include <Logger/StreamLogger.hpp>
#include <Commons/UserInfoManip.hpp>

namespace AdServer
{
namespace Commons
{
  // number of uids in blacklist epected very small, so use default copy constructor
  class UserIdBlackList
  {
    typedef Generics::GnuHashSet<AdServer::Commons::UserId> UserIdBlackListContainer;

  public:
    UserIdBlackList()
    {}

    ~UserIdBlackList()
    {}

    bool is_blacklisted(const AdServer::Commons::UserId& user_id) const noexcept
    {
      return (cont_.find(user_id) != cont_.end());
    }

    bool null_blacklisted(AdServer::Commons::UserId& user_id) const noexcept
    {
      bool blacklisted = is_blacklisted(user_id);
      if (blacklisted)
      {
        user_id = AdServer::Commons::UserId();
      }
      return blacklisted;
    }

    void swap(UserIdBlackList& rhs)
    {
      rhs.cont_.swap(cont_);
    }

    // return false if error occured on load
    template <typename XSDType, typename LoggerType>
    bool load(const XSDType& xsd_config, LoggerType logger, const char* aspect)
    {
      bool res = true;
      if (xsd_config.UserIdBlackList().present())
      {
        std::string user_id_black_list_str =
          xsd_config.UserIdBlackList().get();

        if (!user_id_black_list_str.empty())
        {
          std::string buf;
          std::istringstream oss(user_id_black_list_str);
          while (std::getline(oss, buf))
          {
            String::StringManip::trim(buf, buf);
            if (!buf.empty() && buf[0] != '#')
            {
              try
              {
                AdServer::Commons::UserId uid(buf);
                cont_.insert(uid);
              }
              catch(const eh::Exception& ex)
              {
                Stream::Error ostr;
                ostr << FNS << " can not add UserId: \'" << buf
                     << "\' into UserIdBlackList: eh::Exception: " << ex.what();
                logger->log(ostr.str(),
                  Logging::Logger::WARNING,
                  aspect,
                  "ADS-IMPL-132");
                res = false;
              }
              catch(...)
              {
                Stream::Error ostr;
                ostr << FNS << " can not add UserId: \'" << buf
                     << "\' into UserIdBlackList: unknown exception";
                logger->log(ostr.str(),
                  Logging::Logger::WARNING,
                  aspect,
                  "ADS-IMPL-132");
                res = false;
              }
              buf.clear();
            }
          }
        }
      }
      return res;
    }

  private:
    UserIdBlackListContainer cont_;
  };
}
}
#endif //USERIDBLACKLIST_HPP__
