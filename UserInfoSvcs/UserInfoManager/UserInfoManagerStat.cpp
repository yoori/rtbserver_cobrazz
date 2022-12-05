#include "UserInfoManagerStat.hpp"

namespace
{
  namespace Aspect
  {
    const char USER_INFO_MANAGER_STAT[] = "Statistics";
  }

  namespace Keys
  {
    const Generics::Values::Key USER_COUNT("userProfiles");
    const Generics::Values::Key DAILY_USERS("dailyUsers-Count");
    const Generics::Values::Key BASE_AREA_SIZE("userBaseProfiles-AreaSize");
    const Generics::Values::Key TEMP_AREA_SIZE("userTempProfiles-AreaSize");
    const Generics::Values::Key ADD_AREA_SIZE("userAdditionalProfiles-AreaSize");
    const Generics::Values::Key HISTORY_AREA_SIZE("userHistoryProfiles-AreaSize");
    const Generics::Values::Key FREQ_CAP_AREA_SIZE("userFreqCapProfiles-AreaSize");
    const Generics::Values::Key ALL_AREA_SIZE("userProfiles-AreaSize");
    const Generics::Values::Key ALLOCATOR_CACHE_SIZE("userProfiles-AllocatorCacheSize");
    const Generics::Values::Key AD_CHANNELS("userProfiles-AdChannels");
    const Generics::Values::Key DISCOVER_CHANNELS("userProfiles-DiscoverChannels");

    const Generics::Values::Key FILE_RW_STAT("rwStats");
    const Generics::Values::Key FILE_STAT_TIMESTAMP("rwStatTimestamp");
    const Generics::Values::Key FILE_R_STAT_MAXTIME("readMaxTime");
    const Generics::Values::Key FILE_R_STAT_SUMTIME("readSumTime");
    const Generics::Values::Key FILE_R_STAT_COUNT("readCount");
    const Generics::Values::Key FILE_R_STAT_SIZE("readSize");
    const Generics::Values::Key FILE_W_STAT_MAXTIME("writeMaxTime");
    const Generics::Values::Key FILE_W_STAT_SUMTIME("writeSumTime");
    const Generics::Values::Key FILE_W_STAT_COUNT("writeCount");
    const Generics::Values::Key FILE_W_STAT_SIZE("writeSize");
  }

  template<typename T>
  inline
  CORBACommons::StatsValue
  make_stats_value(
    const Generics::Values::Key& key,
    T value)
  {
    CORBACommons::StatsValue stats;
    stats.key << key.text();
    stats.value <<= value;
    return stats;
  }

  void
  fill(
    const std::vector<CORBACommons::StatsValue>& values,
    CORBACommons::StatsValueSeq& value_seq)
    noexcept
  {
    value_seq.length(values.size());

    for (std::size_t i = 0; i < values.size(); ++i)
    {
      value_seq[i] = values[i];
    }
  }

  std::string
  key_with_index(
    const Generics::Values::Key& key,
    std::size_t inx)
    /*throw(std::exception)*/
  {
    std::ostringstream oss;
    oss << key.text() << '.' << inx;
    return oss.str();
  }
}

namespace AdServer
{
namespace UserInfoSvcs
{
  /*UserInfoManagerStatsImpl*/
  UserInfoManagerStatsImpl::UserInfoManagerStatsImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    UserInfoManagerImpl* uim_val,
    const CORBACommons::CorbaObjectRef& dumper_ref,
    const Generics::Time& update_period)
    noexcept
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      uim_(ReferenceCounting::add_ref(uim_val)),
      update_period_(update_period)
  {
    static const char* FUN = "UserInfoManagerStatsImpl::UserInfoManagerStatsImpl()";

    if(update_period_ != Generics::Time::ZERO)
    {
      // create dump task
      try
      {
        task_runner_ = new Generics::TaskRunner(callback_, 1);
        add_child_object(task_runner_);

        scheduler_ = new Generics::Planner(callback_);
        add_child_object(scheduler_);

        stats_dumper_.reset(new AdServer::Controlling::StatsDumper(dumper_ref));

        Generics::Goal_var msg(new DumpStatsTask(this));
        scheduler_->schedule(
          msg,
          Generics::Time::get_time_of_day() + update_period_);
      }
      catch(const eh::Exception& e)
      {
        uim_->logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER_STAT,
          "ADS-IMPL-83") << FUN <<
          ": failed to init StatsDumper: " << e.what();
      }
    }
  }

  UserInfoManagerStatsImpl::~UserInfoManagerStatsImpl() noexcept
  {}

  CORBACommons::StatsValueSeq*
  UserInfoManagerStatsImpl::get_stats()
    /*throw(CORBA::Exception,
      CORBACommons::ProcessStatsControl::ImplementationException)*/
  {
    static const char* FUN = "UserInfoManagerStatsImpl::get_stats()";

    CORBACommons::StatsValueSeq_var res;
    try
    {
      UserStat user_stat = uim_->get_stats();

      std::vector<CORBACommons::StatsValue> stats{
        make_stats_value(
          Keys::USER_COUNT,
          static_cast<CORBA::ULongLong>(user_stat.users_count)),
        make_stats_value(
          Keys::DAILY_USERS,
          static_cast<CORBA::ULongLong>(user_stat.daily_users)),
        make_stats_value(
          Keys::BASE_AREA_SIZE,
          static_cast<CORBA::ULongLong>(user_stat.base_area_size)),
        make_stats_value(
          Keys::TEMP_AREA_SIZE,
          static_cast<CORBA::ULongLong>(user_stat.temp_area_size)),
        make_stats_value(
          Keys::ADD_AREA_SIZE,
          static_cast<CORBA::ULongLong>(user_stat.add_area_size)),
        make_stats_value(
          Keys::HISTORY_AREA_SIZE,
          static_cast<CORBA::ULongLong>(user_stat.history_area_size)),
        make_stats_value(
          Keys::FREQ_CAP_AREA_SIZE,
          static_cast<CORBA::ULongLong>(user_stat.freq_cap_area_size)),
        make_stats_value(
          Keys::ALL_AREA_SIZE,
          static_cast<CORBA::ULongLong>(user_stat.all_area_size)),
        make_stats_value(
          Keys::ALLOCATOR_CACHE_SIZE,
          static_cast<CORBA::ULongLong>(user_stat.allocator_cache_size)),
        make_stats_value(
          Keys::AD_CHANNELS,
          static_cast<CORBA::ULongLong>(user_stat.ad_channels_count)),
        make_stats_value(
          Keys::DISCOVER_CHANNELS,
          static_cast<CORBA::ULongLong>(user_stat.discover_channels_count))
      };

      const FileRWStats::IntervalStats rw_stats = uim_->get_rw_stats();

      if (rw_stats.empty())
      {
        stats.emplace_back(
          make_stats_value(
            Keys::FILE_RW_STAT,
            "disabled"));
      }
      else
      {
        std::size_t inx = 0;

        for (auto it = rw_stats.begin(); it != rw_stats.end(); ++it, ++inx)
        {
          stats.emplace_back(
            make_stats_value(
              key_with_index(Keys::FILE_STAT_TIMESTAMP, inx),
              it->timestamp.gm_ft()));
          stats.emplace_back(
            make_stats_value(
              key_with_index(Keys::FILE_R_STAT_MAXTIME, inx),
              static_cast<CORBA::ULongLong>(it->read.max_time.microseconds())));
          stats.emplace_back(
            make_stats_value(
              key_with_index(Keys::FILE_R_STAT_SUMTIME, inx),
              static_cast<CORBA::ULongLong>(it->read.sum_time.microseconds())));
          stats.emplace_back(
            make_stats_value(
              key_with_index(Keys::FILE_R_STAT_COUNT, inx),
              static_cast<CORBA::ULongLong>(it->read.count)));
          stats.emplace_back(
            make_stats_value(
              key_with_index(Keys::FILE_R_STAT_SIZE, inx),
              static_cast<CORBA::ULongLong>(it->read.sum_size)));
          stats.emplace_back(
            make_stats_value(
              key_with_index(Keys::FILE_W_STAT_MAXTIME, inx),
              static_cast<CORBA::ULongLong>(it->write.max_time.microseconds())));
          stats.emplace_back(
            make_stats_value(
              key_with_index(Keys::FILE_W_STAT_SUMTIME, inx),
              static_cast<CORBA::ULongLong>(it->write.sum_time.microseconds())));
          stats.emplace_back(
            make_stats_value(
              key_with_index(Keys::FILE_W_STAT_COUNT, inx),
              static_cast<CORBA::ULongLong>(it->write.count)));
          stats.emplace_back(
            make_stats_value(
              key_with_index(Keys::FILE_W_STAT_SIZE, inx),
              static_cast<CORBA::ULongLong>(it->write.sum_size)));
        }
      }

      res = new CORBACommons::StatsValueSeq();
      CORBACommons::StatsValueSeq& res_ref = *res;
      res_ref.length(10);

      res_ref[0].key << Keys::USER_COUNT.text();
      res_ref[0].value <<= static_cast<CORBA::ULongLong>(user_stat.users_count);
      res_ref[1].key << Keys::DAILY_USERS.text();
      res_ref[1].value <<= static_cast<CORBA::ULongLong>(user_stat.daily_users);
      res_ref[2].key << Keys::BASE_AREA_SIZE.text();
      res_ref[2].value <<= static_cast<CORBA::ULongLong>(user_stat.base_area_size);
      res_ref[3].key << Keys::TEMP_AREA_SIZE.text();
      res_ref[3].value <<= static_cast<CORBA::ULongLong>(user_stat.temp_area_size);
      res_ref[4].key << Keys::ADD_AREA_SIZE.text();
      res_ref[4].value <<= static_cast<CORBA::ULongLong>(user_stat.add_area_size);
      res_ref[5].key << Keys::HISTORY_AREA_SIZE.text();
      res_ref[5].value <<=
        static_cast<CORBA::ULongLong>(user_stat.history_area_size);
      res_ref[6].key << Keys::FREQ_CAP_AREA_SIZE.text();
      res_ref[6].value <<=
        static_cast<CORBA::ULongLong>(user_stat.freq_cap_area_size);
      res_ref[7].key << Keys::ALL_AREA_SIZE.text();
      res_ref[7].value <<=
        static_cast<CORBA::ULongLong>(user_stat.all_area_size);
      res_ref[8].key << Keys::AD_CHANNELS.text();
      res_ref[8].value <<=
        static_cast<CORBA::ULongLong>(user_stat.ad_channels_count);
      res_ref[9].key << Keys::DISCOVER_CHANNELS.text();
      res_ref[9].value <<=
        static_cast<CORBA::ULongLong>(user_stat.discover_channels_count);
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserInfoManager::NotReady: " <<
        e.description;
      CORBACommons::throw_desc<
        CORBACommons::ProcessStatsControl::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();

      CORBACommons::throw_desc<
        CORBACommons::ProcessStatsControl::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  void
  UserInfoManagerStatsImpl::dump_stats_() noexcept
  {
    static const char* FUN = "UserInfoManagerStatsImpl::dump_stats_()";

    try
    {
      AdServer::UserInfoSvcs::UserStat user_stat = uim_->get_stats();
      Generics::Values_var snapshot(new Generics::Values);
      snapshot->set(Keys::USER_COUNT, user_stat.users_count);
      snapshot->set(Keys::DAILY_USERS, user_stat.daily_users);
      snapshot->set(Keys::BASE_AREA_SIZE, user_stat.base_area_size);
      snapshot->set(Keys::TEMP_AREA_SIZE, user_stat.temp_area_size);
      snapshot->set(Keys::ADD_AREA_SIZE, user_stat.add_area_size);
      snapshot->set(Keys::HISTORY_AREA_SIZE, user_stat.history_area_size);
      snapshot->set(Keys::FREQ_CAP_AREA_SIZE, user_stat.freq_cap_area_size);
      snapshot->set(Keys::ALL_AREA_SIZE, user_stat.all_area_size);
      snapshot->set(Keys::ALLOCATOR_CACHE_SIZE, user_stat.allocator_cache_size);
      snapshot->set(Keys::AD_CHANNELS, user_stat.ad_channels_count);
      snapshot->set(
        Keys::DISCOVER_CHANNELS,
        user_stat.discover_channels_count);
      stats_dumper_->dump_statistics(*snapshot);
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      // do nothing while UIM not ready
    }
    catch (const eh::Exception& ex)
    {
      uim_->logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER_STAT,
        "ADS-IMPL-82") << FUN <<
        ": dump statistics failed: " << ex.what();
    }

    try
    {
      Generics::Goal_var msg(new DumpStatsTask(this));
      scheduler_->schedule(
        msg,
        Generics::Time::get_time_of_day() + update_period_);
    }
    catch (const eh::Exception& ex)
    {
      uim_->logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER_STAT,
        "ADS-IMPL-82") << "DumpStatsTask::schedule(): failed: " <<
        ex.what();
    }
  }
} /*UserInfoSvcs*/
} /*AdServer*/
