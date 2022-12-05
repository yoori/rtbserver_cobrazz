#ifndef _LOCAL_AUDIENCE_CHANNEL_HPP
#define _LOCAL_AUDIENCE_CHANNEL_HPP

#include <set>
#include <string>
#include <eh/Exception.hpp>
#include <tests/AutoTests/Commons/AdClient.hpp>

namespace AutoTest
{
  class LocalAudienceChannel
  {
    typedef std::set<std::string> Uids;

  public:

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    explicit LocalAudienceChannel(
      const std::string& channel_id,
      const std::string& useropgen_in = std::string());

    void add_user(const std::string& uid, bool auto_commit = false)
      /*throw(Exception)*/;

    void add_user(const AdClient& client, bool auto_commit = false)
      /*throw(Exception)*/;

    template<class Iter>
    void add_users(Iter it, const Iter& end, bool auto_commit = false)
      /*throw(Exception)*/;

    template<class Sequence>
    void add_users(const Sequence& seq, bool auto_commit = false)
      /*throw(Exception)*/;

    bool del_user(const std::string& uid, bool auto_commit = false)
      /*throw(Exception)*/;

    bool del_user(const AdClient& client, bool auto_commit = false)
      /*throw(Exception)*/;

    template<class Iter>
    void del_users(Iter it, const Iter& end, bool auto_commit = false)
      /*throw(Exception)*/;

    template<class Sequence>
    void del_users(const Sequence& seq, bool auto_commit = false)
      /*throw(Exception)*/;

    void fill_random(size_t size = 100, bool auto_commit = false);

    void clear(bool auto_commit = false)
      /*throw(Exception)*/;

    void commit()
      /*throw(Exception)*/;

  private:
    std::string channel_id_;
    std::string useropgen_in_;
    Uids uids_;
  };
};// AutoTest

#endif // _LOCAL_AUDIENCE_CHANNEL_HPP
