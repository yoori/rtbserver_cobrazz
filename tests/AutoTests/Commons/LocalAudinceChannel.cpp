#include "LocalAudinceChannel.hpp"

#include <fstream>

#include "GlobalSettings.hpp"
#include <Generics/Rand.hpp>
#include <Generics/Uuid.hpp>
#include "Traits.hpp"
#include <tests/AutoTests/Commons/Admins/SimpleAdmins.hpp>

namespace AutoTest
{
  namespace
  {
    const char CHARSET[] =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789";

    void unique_name(std::string& templ)
    {
      auto c = templ.rbegin();
      while (*c == 'X')
      {
        size_t rand_i = Generics::safe_rand(sizeof(CHARSET) - 1);
        *c++ = CHARSET[rand_i];
      }
    }
  }

  LocalAudienceChannel::LocalAudienceChannel(
    const std::string& channel_id,
    const std::string& useropgen_in):
    channel_id_(to_string(channel_id)),
    useropgen_in_(),
    uids_()
  {
    if (!useropgen_in.empty())
    {
      useropgen_in_ = useropgen_in;
    }
    else if (GlobalSettings::instance().initialized() &&
             GlobalSettings::instance().config().UserOpGeneratorIn())
    {
      useropgen_in_ =
        GlobalSettings::instance().config().UserOpGeneratorIn()->host() +
        ':' +
        GlobalSettings::instance().config().UserOpGeneratorIn()->path();
    }
  };

  void LocalAudienceChannel::add_user(const std::string& uid,
    bool auto_commit) /*throw(Exception)*/
  {
    uids_.insert(uid);
    if (auto_commit)
    { commit(); }
  }

  void LocalAudienceChannel::add_user(const AdClient& client,
    bool auto_commit) /*throw(Exception)*/
  {
    try
    {
      add_user(
        prepare_uid(client.get_uid(), UUE_ADMIN_PARAMVALUE),
        auto_commit);
    }
    catch(const CookieNotFound&)
    {
      Stream::Error err;
      err << FNS << "user hasn't got uid cookie!";
      throw Exception(err);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err << FNS << "got eh::Exception: " << e.what();
      throw Exception(err);
    }
  }

  template<class Iter>
  void LocalAudienceChannel::add_users(
    Iter it, const Iter& end, bool auto_commit) /*throw(Exception)*/
  {
    for (; it != end; ++it)
    {
      add_user(*it, false);
    }
    if (auto_commit)
    { commit(); }
  }

  template<class Sequence>
  void LocalAudienceChannel::add_users(const Sequence& seq, bool auto_commit)
    /*throw(Exception)*/
  {
    add_users(beginof(seq), endof(seq), auto_commit);
  }

  bool LocalAudienceChannel::del_user(const std::string& uid,
    bool auto_commit) /*throw(Exception)*/
  {
    Uids::iterator pos = uids_.find(uid);
    if (pos != uids_.end())
    {
      uids_.erase(pos);
      if (auto_commit)
      { commit(); }
      return true;
    }
    return false;
  }

  bool LocalAudienceChannel::del_user(const AdClient& client,
    bool auto_commit) /*throw(Exception)*/
  {
    try
    {
      return del_user(
        prepare_uid(client.get_uid(), UUE_ADMIN_PARAMVALUE),
        auto_commit);
    }
    catch(const CookieNotFound&)
    {
      return false;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err << FNS << "got eh::Exception: " << e.what();
      throw Exception(err);
    }
  }

  template<class Iter>
  void LocalAudienceChannel::del_users(
    Iter it, const Iter& end, bool auto_commit) /*throw(Exception)*/
  {
    bool is_changed = false;
    for (it; it != end; ++it)
    {
      is_changed |= del_user(*it, false);
    }
    if (is_changed && auto_commit)
    { commit(); }
  }
  
  template<class Sequence>
  void LocalAudienceChannel::del_users(const Sequence& seq, bool auto_commit)
    /*throw(Exception)*/
  {
    del_users(beginog(seq), endof(seq), auto_commit);
  }

  void LocalAudienceChannel::fill_random(size_t size, bool auto_commit)
  {
    for (size_t i = 0; i < size; ++i)
    {
      add_user(Generics::Uuid::create_random_based().to_string(), false);
    }
    if (auto_commit)
    { commit(); }
  }

  void LocalAudienceChannel::commit() /*throw(Exception)*/
  {
    if (useropgen_in_.empty())
    {
      Stream::Error err;
      err << "LocalAudienceChannel::commit(): "
          << "UserOperationGeneratorIn path must be defined in config "
          << "or in LocalAudienceChannel c-tor.";
      throw Exception(err);
    }

    std::string fullname = "/tmp/channel" + channel_id_ + ".XXXXXX";
    unique_name(fullname);
    std::ofstream file(fullname);

    for (auto it = uids_.begin(); it != uids_.end(); ++it)
    {
      file << *it << std::endl;
    }
    file.close();

    try
    {
      MoveCmd(fullname, useropgen_in_ + "/channel" + channel_id_).exec();
    }
    catch (const eh::Exception& e)
    {
      Stream::Error err;
      err << "LocalAudienceChannel::commit(): "
          << "failed to move uids file to UserOperationGenerator service: "
          << e.what();
      throw Exception(err);
    }
  }

  void LocalAudienceChannel::clear(bool auto_commit)
    /*throw(Exception)*/
  {
    uids_.clear();
    if (auto_commit)
    { commit(); }
  }

}; // namespace AutoTest
