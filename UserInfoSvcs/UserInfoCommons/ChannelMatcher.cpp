#include <sys/types.h>
#include <iostream>
#include <vector>
#include <list>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Stream/MemoryStream.hpp>

#include <Commons/Algs.hpp>

#include "Allocator.hpp"
#include "ChannelMatcher.hpp"

#include "UserProfileUtils.hpp"

namespace AdServer
{
  namespace UserInfoSvcs
  {
    struct DaysVisitsLess
    {
      bool operator()(
        const DaysVisitsReader& left,
        const DaysVisitsReader& right) const
        /*throw(eh::Exception)*/
      {
        return left.days() < right.days();
      }
    };

    struct DaysVisitsMerge
    {
      DaysVisitsWriter operator()(
        const DaysVisitsReader& left,
        const DaysVisitsReader& right) const
        /*throw(eh::Exception)*/
      {
        DaysVisitsWriter ret;
        ret.days() = left.days();
        ret.visits() = left.visits() + right.visits();
        return ret;
      }
    };    

    struct ChannelIdLess
    {
      template<typename LeftType>
      bool operator()(
        const LeftType& left,
        const AudienceChannel& right) const
        /*throw(eh::Exception)*/
      {
        return left.channel_id() < right.channel_id;
      }

      template<typename RightType>
      bool operator()(
        const AudienceChannel& left,
        const RightType& right) const
        /*throw(eh::Exception)*/
      {
        return left.channel_id < right.channel_id();
      }

      template<typename LeftType, typename RightType>
      bool operator()(
        const LeftType& left,
        const RightType& right) const
        /*throw(eh::Exception)*/
      {
        return left.channel_id() < right.channel_id();
      }
    };

    struct ChannelMatchLess
    {
      template<typename LeftType, typename RightType>
      bool operator()(
        const LeftType& left,
        const RightType& right) const
        /*throw(eh::Exception)*/
      {
        return
          (left.channel_id() < right.channel_id() ||
           (left.channel_id() == right.channel_id() &&
            left.channel_trigger_id() < right.channel_trigger_id()));
      }
    };

    struct LastTriggersMerge
    {
      template<typename LeftType, typename RightType>
      LastTriggerWriter operator()(
        const LeftType& left,
        const RightType& right) const
        /*throw(eh::Exception)*/
      {
        LastTriggerWriter ret;
        ret.channel_id() = left.channel_id();
        ret.channel_trigger_id() = left.channel_trigger_id();
        ret.last_match_time() = std::max(left.last_match_time(), right.last_match_time());
        
        return ret;
      }      
    };
    
    struct HistoryChannelInfoMerge
    {
      HistoryChannelInfoWriter operator()(
        const HistoryChannelInfoReader& left,
        const HistoryChannelInfoReader& right) const
        /*throw(eh::Exception)*/
      {
        HistoryChannelInfoWriter ret;
        ret.channel_id() = left.channel_id();
        ret.days_visits().reserve(left.days_visits().size() + right.days_visits().size());

        Algs::merge_unique(
          left.days_visits().begin(), left.days_visits().end(),
          right.days_visits().begin(), right.days_visits().end(),
          std::back_inserter(ret.days_visits()),
          DaysVisitsLess(),
          DaysVisitsMerge());

        return ret;
      }
    };
    
    struct HistoryChannelInfoFilter
    {
      HistoryChannelInfoFilter(const ChannelsHashMap& channels)
        noexcept
        : channels_(channels)
      {}

      bool operator()(const HistoryChannelInfoWriter& obj) const
        /*throw(eh::Exception)*/
      {
        return channels_.find(obj.channel_id()) != channels_.end() &&
          !obj.days_visits().empty();
      }

    private:
      const ChannelsHashMap& channels_;
    };    

    struct ExistChannelFilter
    {
      ExistChannelFilter(const ChannelsHashMap& channels)
        noexcept
        : channels_(channels)
      {}

      template<typename ObjectType>
      bool operator()(const ObjectType& obj) const
        /*throw(eh::Exception)*/
      {
        return channels_.find(obj.channel_id()) != channels_.end();
      }

    private:
      const ChannelsHashMap& channels_;
    };    

    struct HistoryVisitsMerge
    {
      template<typename LeftType, typename RightType>
      HistoryVisitsWriter operator()(
        const LeftType& left,
        const RightType& right) const
        /*throw(eh::Exception)*/
      {
        HistoryVisitsWriter ret;
        ret.channel_id() = left.channel_id();
        ret.visits() = left.visits() + right.visits();
        return ret;
      }      
    };

    struct SessionMatchesMerge
    {
      template<typename LeftType, typename RightType>
      SessionMatchesWriter operator()(
        const LeftType& left,
        const RightType& right) const
        /*throw(eh::Exception)*/
      {
        SessionMatchesWriter ret;
        ret.channel_id() = left.channel_id();
        //ret.timestamps().reserve(left.timestamps().size() + right.timestamps().size());
        std::merge(
          left.timestamps().begin(), left.timestamps().end(),
          right.timestamps().begin(), right.timestamps().end(),
          std::back_inserter(ret.timestamps()));

        return ret;
      }
    };

    struct SessionMatchesFilter
    {
      SessionMatchesFilter() noexcept
      {}
      
      bool operator()(const SessionMatchesWriter& obj) const
        /*throw(eh::Exception)*/
      {
        return !obj.timestamps().empty();
      }
    };

    class SessionMatchesCleaner
    {
    public:
      SessionMatchesCleaner(
        const ChannelsHashMap& channels,
        const Generics::Time& now) noexcept
        : channels_(channels),
          now_(now.tv_sec)
      {}
      
      SessionMatchesWriter operator()(const SessionMatchesWriter& obj) const
        /*throw(eh::Exception)*/
      {
        SessionMatchesWriter ret;
        ret.channel_id() = obj.channel_id();
        ChannelsHashMap::const_iterator ch_it = channels_.find(obj.channel_id());
        if(ch_it != channels_.end())
        {
          check_ts_(ret.timestamps(), obj.timestamps(), ch_it->second->short_intervals);
          ChannelsMatcher::delete_excess_timestamps_(
            ret.timestamps(), ch_it->second->short_intervals);
        }
        return ret;
      }

    private:
      void check_ts_(
        SessionMatchesWriter::timestamps_Container& out,
        const SessionMatchesWriter::timestamps_Container& in,
        const ChannelIntervalList& cil) const noexcept
      {
        long max_time_to = cil.max_time_to();

        for (SessionMatchesWriter::timestamps_Container::
               const_iterator in_it = in.begin();
             in_it != in.end(); ++in_it)
        {
          if (now_ - *in_it <= max_time_to)
          {
            out.push_back(*in_it);
          }
        }
      }
      
    private:
      const ChannelsHashMap& channels_;
      long now_;
    };

    class HistoryChannelInfoCleaner
    {
    public:
      HistoryChannelInfoCleaner(const ChannelsHashMap& channels)
        : channels_(channels)
      {}
      
      HistoryChannelInfoWriter
      operator()(const HistoryChannelInfoWriter& writer)
      {
        HistoryChannelInfoWriter ret;
        ret.channel_id() = writer.channel_id();
        
        ChannelsHashMap::const_iterator cit = channels_.find(writer.channel_id());

        if (cit != channels_.end())
        {
          unsigned long max_time_to_in_days = std::max(
            cit->second->long_intervals.max_time_to(),
            cit->second->today_long_intervals.max_time_to()) / SEC_IN_DAY;

          ret.days_visits().reserve(writer.days_visits().size());

          for (HistoryChannelInfoWriter::days_visits_Container::const_iterator it =
                 writer.days_visits().begin();
               it != writer.days_visits().end(); ++it)
          {
            if((*it).days() < max_time_to_in_days)
            {
              ret.days_visits().push_back(*it);
            }
          }
        }
        
        return ret;
      }

    private:
      const ChannelsHashMap& channels_;
    };

    class HistoryChannelInfoHistoryVisitsMerge
    {
    public:
      HistoryChannelInfoHistoryVisitsMerge(unsigned long days_offset)
        : days_offset_(days_offset)
      {}
      
      HistoryChannelInfoWriter
      operator()(
        const HistoryChannelInfoReader& left,
        const HistoryVisitsReader& right)
        const noexcept
      {
        HistoryChannelInfoWriter ret;
        ret.channel_id() = left.channel_id();

        DaysVisitsWriter dvw;
        dvw.days() = days_offset_;
        dvw.visits() = right.visits();

        HistoryChannelInfoReader::days_visits_Container::const_iterator it =
          left.days_visits().begin();
        if (it != left.days_visits().end() && (*it).days() == 0)
        {
          dvw.visits() += (*it).visits();
          ++it;
        }

        ret.days_visits().reserve(left.days_visits().size() + 1);

        ret.days_visits().push_back(dvw);

        for (; it != left.days_visits().end(); ++it)
        {
          dvw.days() = (*it).days() + days_offset_;
          dvw.visits() = (*it).visits();
          ret.days_visits().push_back(dvw);
        }

        return ret;
      }
      
    private:
      unsigned long days_offset_;
    };

    class HistoryChannelInfoConverter
    {
    public:
      HistoryChannelInfoConverter(unsigned long days_offset)
        : days_offset_(days_offset)
      {}

      const HistoryChannelInfoWriter&
      operator()(const HistoryChannelInfoWriter& ret) const
        noexcept
      {
        return ret;
      }
      
      HistoryChannelInfoWriter
      operator()(const HistoryChannelInfoReader& reader) const
        noexcept
      {
        HistoryChannelInfoWriter ret;
        ret.channel_id() = reader.channel_id();

        DaysVisitsWriter dvw;

        ret.days_visits().reserve(reader.days_visits().size());

        for (HistoryChannelInfoReader::days_visits_Container::const_iterator it =
               reader.days_visits().begin();
            it != reader.days_visits().end(); ++it)
        {
          dvw.days() = (*it).days() + days_offset_;
          dvw.visits() = (*it).visits();
          ret.days_visits().push_back(dvw);
        }

        return ret;
      }

      HistoryChannelInfoWriter
      operator()(
        const HistoryVisitsReader& right)
        const noexcept
      {
        HistoryChannelInfoWriter ret;
        ret.channel_id() = right.channel_id();
        
        DaysVisitsWriter dvw;
        dvw.days() = days_offset_;
        dvw.visits() = right.visits();
        ret.days_visits().push_back(dvw);

        return ret;
      }

    private:
      unsigned long days_offset_;
    };
      
    class AudienceChannelConverter
    {
    public:
      AudienceChannelConverter()
      {}

      const AudienceChannelWriter&
      operator()(const AudienceChannelWriter& ret) const
        noexcept
      {
        return ret;
      }

      AudienceChannelWriter
      operator()(
        const AudienceChannel& init)
        const noexcept
      {
        AudienceChannelWriter ret;
        ret.channel_id() = init.channel_id;
        ret.time() = init.time.tv_sec;
        return ret;
      }
    };

    struct AudienceChannelMerge
    {
      AudienceChannelWriter
      operator()(
        const AudienceChannelReader& left,
        const AudienceChannel& right) const
        /*throw(eh::Exception)*/
      {
        AudienceChannelWriter ret;
        ret.channel_id() = left.channel_id();
        ret.time() = std::max(left.time(), static_cast<uint32_t>(right.time.tv_sec));
        return ret;
      }
    };

    /** ChannelsMatcher */
    ChannelsMatcher::ChannelsMatcher()
    {}
    
    ChannelsMatcher::ChannelsMatcher(
      Generics::SmartMemBuf* base_profile,
      Generics::SmartMemBuf* add_profile) noexcept
      : base_profile_(ReferenceCounting::add_ref(base_profile)),
        add_profile_(ReferenceCounting::add_ref(add_profile))
    {}

    void
    ChannelsMatcher::unique_channels(
      const Generics::MemBuf& base_profile,
      const Generics::MemBuf* history_profile,
      const ChannelDictionary& dictionary,
      UniqueChannelsResult& ucr)
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::unique_channels()";

      AllUniqueChannels uniq_ids;
      
      try
      {
        collect_channel_ids_(base_profile, dictionary, uniq_ids);

        if(history_profile)
        {
          collect_history_channel_ids_(*history_profile, dictionary, uniq_ids);
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": " <<
          "Attempt to get unique channels from invalid profile: " <<
          ex.what();
        throw InvalidProfileException(ostr);
      }
      
      ucr.init(uniq_ids);
    }

    bool
    ChannelsMatcher::need_channel_count_stats_logging(
      const Generics::Time& now,
      const Generics::Time& gmt_offset) const
      /*throw(InvalidProfileException)*/
    {
      if (!base_profile_->membuf().empty() && add_profile_->membuf().empty())
      {
        ChannelsProfileReader rdr(
          base_profile_->membuf().data(),
          base_profile_->membuf().size());

        if ((now + gmt_offset).get_gm_time().get_date() >
            (Generics::Time(
               rdr.last_request_time()) + gmt_offset).get_gm_time().get_date())
          {
            return true;
          }
      }

      return false;
    }
    
    Generics::Time
    ChannelsMatcher::last_request() const
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::last_request()";
      
      if (base_profile_->membuf().empty())
      {
        return Generics::Time::ZERO;
      }

      try
      {
        ChannelsProfileReader rdr(
          base_profile_->membuf().data(),
          base_profile_->membuf().size());

        return Generics::Time(rdr.last_request_time());
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": " <<
          "Attempt to get last request time from invalid profile: " <<
          ex.what();
        throw InvalidProfileException(ostr);
      }

      return Generics::Time::ZERO;
    }

    Generics::Time
    ChannelsMatcher::create_time() const
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::create_time()";
      
      if (base_profile_->membuf().empty())
      {
        return Generics::Time::ZERO;
      }

      try
      {
        ChannelsProfileReader rdr(
          base_profile_->membuf().data(),
          base_profile_->membuf().size());

        return Generics::Time(rdr.create_time());
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": " <<
          "Attempt to get create time from invalid profile: " <<
          ex.what();
        throw InvalidProfileException(ostr);
      }

      return Generics::Time::ZERO;
    }

    Generics::Time
    ChannelsMatcher::session_start() const
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::session_start()";

      try
      {
        if (!add_profile_->membuf().empty())
        {
          ChannelsProfileReader rdr(
            add_profile_->membuf().data(),
            add_profile_->membuf().size());
          
          return Generics::Time(rdr.session_start());
        }
        else if (!base_profile_->membuf().empty())
        {
          ChannelsProfileReader rdr(
            base_profile_->membuf().data(),
            base_profile_->membuf().size());
          
          return Generics::Time(rdr.session_start());
        }
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": " <<
          "Attempt to get session start time from invalid profile: " <<
          ex.what();
        throw InvalidProfileException(ostr);
      }

      return Generics::Time::ZERO;
    }

    bool ChannelsMatcher::fraud_user(const Generics::Time& now)
    {
      ChannelsProfileWriter writer;
      bool resave = false;

      if(base_profile_->membuf().size() != 0)
      {
        ChannelsProfileReader reader(
          base_profile_->membuf().data(),
          base_profile_->membuf().size());

        // fraud detection requests isn't serialized - use greatest
        if(now.tv_sec > reader.ignore_fraud_time())
        {
          writer.init(
            base_profile_->membuf().data(),
            base_profile_->membuf().size());

          writer.ignore_fraud_time() = now.tv_sec;
          resave = true;
        }
      }
      else
      {
        writer.version() = CURRENT_BASE_PROFILE_VERSION;
        writer.create_time() = now.tv_sec;
        writer.history_time() = now.tv_sec;
        writer.ignore_fraud_time() = now.tv_sec;
        writer.last_request_time() = 0;
        writer.session_start() = 0;
        writer.household() = 0;
        writer.first_colo_id() = UNKNOWN_COLO_ID;
        writer.last_colo_id() = UNKNOWN_COLO_ID;
        
        resave = true;
      }

      if(resave)
      {
        unsigned long save_size = writer.size();
        base_profile_->membuf().alloc(save_size);
        writer.save(base_profile_->membuf().data(), save_size);
      }

      return resave;
    }

    void
    ChannelsMatcher::write_geo_data_(
      GeoDataWriter& gdw,
      const CoordData& coord_data,
      const Generics::Time& now)
    {
      if (coord_data.defined)
      {
        gdw.latitude().alloc(
          AdServer::CampaignSvcs::CoordDecimal::PACK_SIZE);
        coord_data.latitude.pack(gdw.latitude().data());
       
        gdw.longitude().alloc(
          AdServer::CampaignSvcs::CoordDecimal::PACK_SIZE);
        coord_data.longitude.pack(gdw.longitude().data());

        gdw.accuracy().alloc(
          AdServer::CampaignSvcs::AccuracyDecimal::PACK_SIZE);
        coord_data.accuracy.pack(gdw.accuracy().data());

        gdw.timestamp() = now.tv_sec;
      }
    }

    AdServer::CampaignSvcs::AccuracyDecimal
    ChannelsMatcher::read_accuracy_(
      const void* buf)
    {
      AdServer::CampaignSvcs::AccuracyDecimal res;
      res.unpack(buf);

      return res;
    }

    AdServer::CampaignSvcs::CoordDecimal
    ChannelsMatcher::read_coord_(
      const void* buf)
    {
      AdServer::CampaignSvcs::CoordDecimal res;
      res.unpack(buf);

      return res;
    }
    
    template <typename _T1, typename _T2>
    void
    ChannelsMatcher::merge_geo_data_(
      ChannelsProfileWriter::geo_data_Container& res_geo_data,
      const _T1& base_geo_data,
      const _T2& add_geo_data,
      const Generics::Time& now)
    {
      unsigned long geo_data_count = 0;
      typename _T1::const_iterator base_it = base_geo_data.begin();
      typename _T2::const_iterator add_it = add_geo_data.begin();

      while (base_it != base_geo_data.end() &&
             add_it != add_geo_data.end() &&
             geo_data_count < MAX_GEO_DATA)
      {
        if ((*base_it).timestamp() > (*add_it).timestamp())
        {
          if ((*base_it).timestamp() / SEC_IN_DAY <
              now.tv_sec / SEC_IN_DAY - 15)
          {
            base_it = base_geo_data.end();
            add_it = add_geo_data.end();
            
            break;
          }
          
          res_geo_data.push_back(*base_it);
          
          ++base_it;
          ++geo_data_count;
        }
        else if ((*base_it).timestamp() < (*add_it).timestamp())
        {
          if ((*add_it).timestamp() / SEC_IN_DAY <
              now.tv_sec / SEC_IN_DAY - 15)
          {
            base_it = base_geo_data.end();
            add_it = add_geo_data.end();
            
            break;
          }
          
          res_geo_data.push_back(*add_it);
          
          ++add_it;
          ++geo_data_count;
        }
        else
        {
          if ((*add_it).timestamp() / SEC_IN_DAY <
              now.tv_sec / SEC_IN_DAY - 15)
          {
            base_it = base_geo_data.end();
            add_it = add_geo_data.end();
            
            break;
          }
          
          if (read_coord_((*base_it).latitude().get()) ==
              read_coord_((*add_it).latitude().get()) &&
              read_coord_((*base_it).longitude().get()) ==
              read_coord_((*add_it).longitude().get()))
          {
            if (read_accuracy_((*base_it).latitude().get()) <
                read_accuracy_((*add_it).latitude().get()))
            {
              res_geo_data.push_back(*base_it);
            }
            else
            {
              res_geo_data.push_back(*add_it);
            }
            
            ++base_it;
            ++add_it;
            ++geo_data_count;
          }
          else
          {
            res_geo_data.push_back(*base_it);
            
            ++base_it;
            ++geo_data_count;
          }
        }
      }
      
      while (base_it != base_geo_data.end() && geo_data_count < MAX_GEO_DATA)
      {
        res_geo_data.push_back(*base_it);
        
        ++base_it;
        ++geo_data_count;
      }
      
      while (add_it != add_geo_data.end() && geo_data_count < MAX_GEO_DATA)
      {
        res_geo_data.push_back(*add_it);
        
        ++add_it;
        ++geo_data_count;
      }
    }

    template <typename _T1, typename _T2>
    void
    ChannelsMatcher::add_geo_data_(
      ChannelsProfileWriter::geo_data_Container& res_geo_data,
      const _T1& geo_data_array,
      const _T2& new_geo_data)
    {
      unsigned long geo_data_count = 0;
      bool new_geo_data_inserted = false;

      auto g_it = geo_data_array.begin();

      while (g_it != geo_data_array.end())
      {
        if (!new_geo_data_inserted &&
            (*g_it).timestamp() <= new_geo_data.timestamp())
        {
          if (!((*g_it).timestamp() == new_geo_data.timestamp() &&
              read_coord_(new_geo_data.longitude().data()) ==
                read_coord_((*g_it).longitude().get()) &&
              read_coord_(new_geo_data.latitude().data()) ==
                read_coord_((*g_it).latitude().get()) &&
              read_coord_(new_geo_data.accuracy().data()) ==
                read_coord_((*g_it).accuracy().get())))
          {
            res_geo_data.push_back(new_geo_data);
            ++geo_data_count;
          }
          
          new_geo_data_inserted = true;
        }

        if (geo_data_count < MAX_GEO_DATA)
        {
          if ((*g_it).timestamp() / SEC_IN_DAY <
              new_geo_data.timestamp() / SEC_IN_DAY - 15)
          {//already insered new data
            break;
          }

          res_geo_data.push_back(*g_it);

          ++g_it;
          ++geo_data_count;
        }
        else
        {
          break;
        }
      }
      
      if (!new_geo_data_inserted && geo_data_count < MAX_GEO_DATA)
      {
        res_geo_data.push_back(new_geo_data);
      }
    }
    
    bool
    ChannelsMatcher::merge_triggers_section_(
      ChannelIdVector& result_filtered_channels,
      ChannelsProfileWriter::last_page_triggers_Container& last_triggers,
      const ChannelMatchVector& triggered_channels,
      const ChannelsProfileReader::last_page_triggers_Container& last_triggers_rdr,
      const ChannelsHashMap& channels,
      const Generics::Time& now,
      const Generics::Time& repeat_trigger_timeout,
      bool filter_contextual_triggers)
    {
      bool res = false;

      std::set<unsigned long> result_channels_set;
      uint32_t time_offset = (now - repeat_trigger_timeout).tv_sec;

      last_triggers.reserve(last_triggers_rdr.size() + triggered_channels.size());
      ChannelsProfileReader::last_page_triggers_Container::const_iterator rdr_it =
        last_triggers_rdr.begin();
      ChannelMatchVector::const_iterator match_it = triggered_channels.begin();

      if (repeat_trigger_timeout == Generics::Time::ZERO)
      {
        while (match_it != triggered_channels.end())
        {
          ChannelsHashMap::const_iterator cit = channels.find(match_it->channel_id);
          
          if (cit != channels.end())
          {
            result_channels_set.insert(match_it->channel_id);

            res = true;
          }
          
          ++match_it;
        }
      }
      else
      {
        while (rdr_it != last_triggers_rdr.end() &&
               match_it != triggered_channels.end())
        {
          if (*rdr_it < *match_it)
          {
            if (channels.find((*rdr_it).channel_id()) != channels.end() &&
                time_offset < (*rdr_it).last_match_time())
            {
              last_triggers.push_back(*rdr_it);
            }
            
            ++rdr_it;
          }
          else if (*rdr_it == *match_it)
          {
            ChannelsHashMap::const_iterator cit = channels.find((*rdr_it).channel_id());
            
            if (cit != channels.end())
            {
              if ((*rdr_it).channel_trigger_id() != 0)
              {
                LastTriggerWriter wr;
                wr.channel_id() = (*rdr_it).channel_id();
                wr.channel_trigger_id() = (*rdr_it).channel_trigger_id();
                wr.last_match_time() =
                  time_offset >= (*rdr_it).last_match_time() ?
                  now.tv_sec :
                  (*rdr_it).last_match_time();
                
                last_triggers.push_back(wr);
              }
              
              if ((!filter_contextual_triggers && cit->second->contextual) ||
                  time_offset >= (*rdr_it).last_match_time() ||
                  match_it->channel_trigger_id == 0)
              {
                result_channels_set.insert(match_it->channel_id);
                
                res = true;
              }
            }
            
            ++match_it;
            ++rdr_it;
          }
          else
          {
            ChannelsHashMap::const_iterator cit = channels.find(match_it->channel_id);
            
            if (cit != channels.end())
            {
              if (!cit->second->contextual && match_it->channel_trigger_id != 0)
              {
                LastTriggerWriter wr;
                wr.channel_id() = match_it->channel_id;
                wr.channel_trigger_id() = match_it->channel_trigger_id;
                wr.last_match_time() = now.tv_sec;
                
                last_triggers.push_back(wr);
              }
              
              result_channels_set.insert(match_it->channel_id);
              
              res = true;
            }
            
            ++match_it;
          }
        }
        
        while (rdr_it != last_triggers_rdr.end())
        {
          if (channels.find((*rdr_it).channel_id()) != channels.end() &&
              time_offset < (*rdr_it).last_match_time())
          {
            last_triggers.push_back(*rdr_it);
          }
          
          ++rdr_it;
        }
        
        while (match_it != triggered_channels.end())
        {
          ChannelsHashMap::const_iterator cit = channels.find(match_it->channel_id);
          
          if (cit != channels.end())
          {
            if (!cit->second->contextual && match_it->channel_trigger_id != 0)
            {
              LastTriggerWriter wr;
              wr.channel_id() = match_it->channel_id;
              wr.channel_trigger_id() = match_it->channel_trigger_id;
              wr.last_match_time() = now.tv_sec;
              
              last_triggers.push_back(wr);
            }
            
            result_channels_set.insert(match_it->channel_id);
            
            res = true;
          }
          
          ++match_it;
        }
      }
      
      result_filtered_channels.assign(result_channels_set.begin(), result_channels_set.end());
      
      return res;
    }

    bool
    ChannelsMatcher::update_triggers_(
      ChannelIdPack& result_filtered_channels,
      ChannelsProfileWriter& result_profile_triggers,
      const ChannelMatchPack& triggered_channels,
      const ChannelsProfileReader* profile_reader,
      const ChannelDictionary& channels,
      const Generics::Time& now,
      const Generics::Time& repeat_trigger_timeout,
      bool filter_contextual_triggers)
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::update_triggers_()";

      bool res = false;

      try
      {
        result_filtered_channels.persistent_channels =
          triggered_channels.persistent_channels;

        ChannelsProfileReader::last_page_triggers_Container last_page_triggers;
        ChannelsProfileReader::last_search_triggers_Container last_search_triggers;
        ChannelsProfileReader::last_url_triggers_Container last_url_triggers;
        ChannelsProfileReader::last_url_keyword_triggers_Container last_url_keyword_triggers;
        
        res |= merge_triggers_section_(
          result_filtered_channels.page_channels,
          result_profile_triggers.last_page_triggers(),
          triggered_channels.page_channels,
          profile_reader != 0 ?
            (*profile_reader).last_page_triggers() :
            last_page_triggers,
          channels.page_channels,
          now,
          repeat_trigger_timeout,
          filter_contextual_triggers);
        
        res |= merge_triggers_section_(
          result_filtered_channels.search_channels,
          result_profile_triggers.last_search_triggers(),
          triggered_channels.search_channels,
          profile_reader != 0 ?
            (*profile_reader).last_search_triggers() :
            last_search_triggers,
          channels.search_channels,
          now,
          repeat_trigger_timeout,
          filter_contextual_triggers);
        
        res |= merge_triggers_section_(
          result_filtered_channels.url_channels,
          result_profile_triggers.last_url_triggers(),
          triggered_channels.url_channels,
          profile_reader != 0 ?
            (*profile_reader).last_url_triggers() :
            last_url_triggers,
          channels.url_channels,
          now,
          repeat_trigger_timeout,
          filter_contextual_triggers);

        res |= merge_triggers_section_(
          result_filtered_channels.url_keyword_channels,
          result_profile_triggers.last_url_keyword_triggers(),
          triggered_channels.url_keyword_channels,
          profile_reader != 0 ?
            (*profile_reader).last_url_keyword_triggers() :
            last_url_keyword_triggers,
          channels.url_keyword_channels,
          now,
          repeat_trigger_timeout,
          filter_contextual_triggers);
        
        return res;
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Attempt to match invalid profile: " << ex.what();
        throw InvalidProfileException(ostr);
      }
      
      return false;
    }

    void
    ChannelsMatcher::add_audience_channels(
      const AudienceChannelSet& audience_channels,
      const ChannelDictionary& channels,
      const Generics::Time& expire_time)
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::add_audience_channels()";

      if (audience_channels.empty())
      {
        return;
      }
      
      try
      {
        // if profile expired by last request, clear its content
        // excluding audience channels
        ChannelsProfileWriter::audience_channels_Container new_audience_channels;

        bool init_profile = true;

        if (base_profile_->membuf().size() != 0)
        {
          ChannelsProfileReader reader(
            base_profile_->membuf().data(),
            base_profile_->membuf().size());

          if(reader.last_request_time() == 0 ||
            Generics::Time(reader.last_request_time()) > expire_time)
          {
            init_profile = false;
          }

          new_audience_channels.reserve(
            new_audience_channels.size() + reader.audience_channels().size() + audience_channels.size());

          Algs::merge_unique(
            reader.audience_channels().begin(),
            reader.audience_channels().end(),
            audience_channels.begin(),
            audience_channels.end(),
            Algs::modify_inserter(
              Algs::filter_inserter( // filter empty channels
                std::back_inserter(new_audience_channels),
                ExistChannelFilter(channels.audience_channels)),
              AudienceChannelConverter()),
            ChannelIdLess(),
            AudienceChannelMerge());
        }
        else
        {
          new_audience_channels.reserve(
            new_audience_channels.size() + audience_channels.size());

          std::copy(
            audience_channels.begin(),
            audience_channels.end(),
            Algs::modify_inserter(
              Algs::filter_inserter( // filter empty channels
                std::back_inserter(new_audience_channels),
                ExistChannelFilter(channels.audience_channels)),
              AudienceChannelConverter()));
        }

        ChannelsProfileWriter writer;

        if(init_profile)
        {
          writer.version() = CURRENT_BASE_PROFILE_VERSION;
          writer.create_time() = 0;//now.tv_sec;
          writer.history_time() = 0;//now.tv_sec;
          writer.ignore_fraud_time() = 0;//now.tv_sec;
          writer.last_request_time() = 0;
          writer.session_start() = 0;
          writer.household() = 0;
          writer.first_colo_id() = UNKNOWN_COLO_ID;
          writer.last_colo_id() = UNKNOWN_COLO_ID;
        }
        else
        {
          writer.init(
            base_profile_->membuf().data(),
            base_profile_->membuf().size());          
        }

        writer.audience_channels().swap(new_audience_channels);

        base_profile_->membuf().alloc(writer.size());
        writer.save(base_profile_->membuf().data(), writer.size());
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": " << ex.what();
        throw InvalidProfileException(ostr);
      }
    }

    void ChannelsMatcher::remove_audience_channels(
      const AudienceChannelSet& audience_channels)
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::remove_audience_channels()";
      
      if (audience_channels.empty() || base_profile_->membuf().size() == 0)
      {
        return;
      }

      try
      {
        ChannelsProfileWriter writer;
        writer.init(
          base_profile_->membuf().data(),
          base_profile_->membuf().size());

        ChannelsProfileWriter::audience_channels_Container::iterator w_it =
          writer.audience_channels().begin();
        ChannelsProfileWriter::audience_channels_Container::iterator end_w_it =
          writer.audience_channels().end();

        ChannelsProfileWriter::audience_channels_Container new_audience_channels;
        new_audience_channels.reserve(writer.audience_channels().size());

        AudienceChannelSet::const_iterator c_it = audience_channels.begin();

        while (w_it != end_w_it && c_it != audience_channels.end())
        {
          if ((*w_it).channel_id() < c_it->channel_id)
          {
            new_audience_channels.push_back(*w_it);
            ++w_it;
          }
          else if ((*w_it).channel_id() > c_it->channel_id)
          {
            ++c_it;
          }
          else
          {
            if ((*w_it).time() <= (c_it->time).tv_sec)
            {
              // remove audience channel
            }
            else
            {
              new_audience_channels.push_back(*w_it);
              ++w_it;
            }
            
            ++c_it;
          }
        }

        std::copy(w_it, end_w_it, std::back_inserter(new_audience_channels));
        writer.audience_channels().swap(new_audience_channels);

        base_profile_->membuf().alloc(writer.size());
        writer.save(base_profile_->membuf().data(), writer.size());
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": " << ex.what();
        throw InvalidProfileException(ostr);
      }
    }

    void ChannelsMatcher::match(
      ChannelMatchMap& result_channels,
      const Generics::Time& now,
      const ChannelMatchPack& channels_pack,
      const ChannelDictionary& channels,
      const ProfileMatchParams& profile_match_params,
      ProfileProperties& properties,
      const Generics::Time& session_timeout,
      bool match_to_add)
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::match()";

      bool base_exists = false;
      bool add_exists = false;

      no_result_ = profile_match_params.no_result;

      ChannelsInfoReader base_page_channels(0, 0);
      ChannelsInfoReader base_search_channels(0, 0);
      ChannelsInfoReader base_url_channels(0, 0);
      ChannelsInfoReader base_url_keyword_channels(0, 0);
      ChannelsProfileReader::audience_channels_Container base_audience_channels;
      
      PersistentMatchesReader base_persistent_matches(0, 0);

      ChannelsProfileReader::geo_data_Container base_geo_data;

      uint32_t base_last_request_time = 0;
      uint32_t base_ignore_fraud_time = 0;
      uint32_t base_create_time = 0;
      uint32_t base_history_time = 0;
      uint32_t base_session_start = 0;
      uint32_t base_household = 0;
      uint32_t base_first_colo_id = profile_match_params.request_colo_id;
      std::string base_cohort;

      if (base_profile_->membuf().size() != 0)
      {
        base_exists = true;
        
        ChannelsProfileReader base_rdr(
          base_profile_->membuf().data(),
          base_profile_->membuf().size());

        base_last_request_time = base_rdr.last_request_time();
        base_ignore_fraud_time = base_rdr.ignore_fraud_time();
        base_create_time = base_rdr.create_time();
        base_history_time = base_rdr.history_time();
        base_session_start = base_rdr.session_start();

        base_page_channels = base_rdr.page_channels();
        base_search_channels = base_rdr.search_channels();
        base_url_channels = base_rdr.url_channels();
        base_url_keyword_channels = base_rdr.url_keyword_channels();
        
        base_persistent_matches = base_rdr.persistent_matches();
        base_household = base_rdr.household();

        base_first_colo_id = base_rdr.first_colo_id();

        base_cohort = base_rdr.cohort();

        base_geo_data = base_rdr.geo_data();

        base_audience_channels = base_rdr.audience_channels();
        
        if (!no_result_)
        {
          for (ChannelsProfileReader::audience_channels_Container::const_iterator it =
                 base_rdr.audience_channels().begin();
               it != base_rdr.audience_channels().end(); ++it)
          {
            ChannelsHashMap::const_iterator c_it =
              channels.audience_channels.find((*it).channel_id());
            if (c_it != channels.audience_channels.end())
            {
              result_channels[(*it).channel_id()] =
                (c_it->second->today_long_intervals.begin())->weight;
            }
          }
        }
      }
      
      Generics::Time current_time(
        std::max(static_cast<uint32_t>(now.tv_sec),
                 base_last_request_time));
      
      properties.fraud_request =
        now.tv_sec <= base_ignore_fraud_time;
      
      ChannelsProfileWriter upw;
      upw.version() = CURRENT_BASE_PROFILE_VERSION;
      upw.create_time() = 0;
      upw.history_time() = 0;
      upw.ignore_fraud_time() = 0;
      upw.last_request_time() = current_time.tv_sec;
      upw.session_start() = 0;
      
      /* fill result profile */
      ChannelsProfileWriter res_upw;
      res_upw.version() = CURRENT_BASE_PROFILE_VERSION;
      res_upw.create_time() = now.tv_sec;
      res_upw.history_time() = now.tv_sec;
      res_upw.ignore_fraud_time() = 0;
      res_upw.last_request_time() = current_time.tv_sec;
      res_upw.session_start() = 0;
      res_upw.household() =
        (base_profile_->membuf().size() != 0) ?
         base_household : profile_match_params.household;

       ChannelIdPack matched_channels;
       ChannelsProfileReader base_rdr =
         ChannelsProfileReader(
           base_profile_->membuf().data(),
           base_profile_->membuf().size());
       ChannelsProfileReader add_rdr =
         ChannelsProfileReader(
           base_profile_->membuf().data(),
           base_profile_->membuf().size());

       ChannelsProfileReader* rdr_ptr = 0;
       if (match_to_add && add_exists)
       {
         rdr_ptr = &add_rdr;
       }
       else if (base_exists)
       {
         rdr_ptr = &base_rdr;
       }

       if (true)
       {
         update_triggers_(
           matched_channels,
           res_upw,
           channels_pack,
           rdr_ptr,
           channels,
           current_time,
           profile_match_params.repeat_trigger_timeout,
           profile_match_params.filter_contextual_triggers);
       }

       ChannelsInfoReader add_page_channels(0, 0);
       ChannelsInfoReader add_search_channels(0, 0);
       ChannelsInfoReader add_url_channels(0, 0);
       ChannelsInfoReader add_url_keyword_channels(0, 0);

       PersistentMatchesReader add_persistent_matches(0, 0);

       ChannelsProfileReader::geo_data_Container add_geo_data;

       uint32_t add_last_request_time = 0;
       uint32_t add_create_time = 0;
       uint32_t add_session_start = 0;
       uint32_t add_first_colo_id = profile_match_params.request_colo_id;
       std::string add_cohort;

       if (add_profile_->membuf().size() != 0)
       {
         add_exists = true;

         ChannelsProfileReader add_rdr(
           add_profile_->membuf().data(),
           add_profile_->membuf().size());

         add_last_request_time = add_rdr.last_request_time();
         add_create_time = add_rdr.create_time();

         add_page_channels = add_rdr.page_channels();
         add_search_channels = add_rdr.search_channels();
         add_url_channels = add_rdr.url_channels();
         add_url_keyword_channels = add_rdr.url_keyword_channels();

         add_persistent_matches = add_rdr.persistent_matches();

         add_first_colo_id = add_rdr.first_colo_id();

         add_cohort = add_rdr.cohort();

         add_geo_data = add_rdr.geo_data();
       }

       if (match_to_add)
       {
         res_upw.create_time() = add_create_time == 0 ?
           current_time.tv_sec : add_create_time;

         res_upw.last_request_time() =
           profile_match_params.change_last_request ?
             current_time.tv_sec : add_last_request_time;

         res_upw.first_colo_id() =
           (add_first_colo_id != UNKNOWN_COLO_ID && add_first_colo_id != DEFAULT_COLO) ?
           add_first_colo_id :
           profile_match_params.request_colo_id;

         res_upw.cohort() = add_cohort;

         set_cohort_(
           res_upw.cohort(),
           profile_match_params.cohort,
           profile_match_params.cohort2);

         /*
         if (profile_match_params.coord_data.defined)
         {
           GeoDataWriter new_geo_data;
           write_geo_data_(
             new_geo_data,
             profile_match_params.coord_data,
             current_time);

           add_geo_data_(
             res_upw.geo_data(),
             add_geo_data,
             new_geo_data);
         }
         else
         {
           for (ChannelsProfileReader::geo_data_Container::const_iterator it =
                  add_geo_data.begin(); it != add_geo_data.end(); ++it)
           {
             res_upw.geo_data().push_back(*it);
           }
         }
         */
       }
       else
       {
         res_upw.create_time() = base_create_time == 0 ?
           current_time.tv_sec : base_create_time;

         res_upw.last_request_time() =
           profile_match_params.change_last_request ?
             current_time.tv_sec : base_last_request_time;

         res_upw.first_colo_id() =
           (base_first_colo_id != UNKNOWN_COLO_ID && base_first_colo_id != DEFAULT_COLO) ?
           base_first_colo_id :
           profile_match_params.request_colo_id;

         res_upw.cohort() = base_cohort;

         set_cohort_(
           res_upw.cohort(),
           profile_match_params.cohort,
           profile_match_params.cohort2);

         /*
         if (profile_match_params.coord_data.defined)
         {
           GeoDataWriter new_geo_data;
           write_geo_data_(
             new_geo_data,
             profile_match_params.coord_data,
             current_time);

           add_geo_data_(
             res_upw.geo_data(),
             base_geo_data,
             new_geo_data);
         }
         else
         {
           for (ChannelsProfileReader::geo_data_Container::const_iterator it =
                  base_geo_data.begin(); it != base_geo_data.end(); ++it)
           {
             res_upw.geo_data().push_back(*it);
           }
         }
         */
       }

       split_cohort_(
         properties.cohort,
         properties.cohort2,
         res_upw.cohort());

       res_upw.last_colo_id() = profile_match_params.request_colo_id;

       res_upw.history_time() = base_create_time == 0 ?
         current_time.tv_sec : base_history_time;
       res_upw.ignore_fraud_time() = base_ignore_fraud_time;

       unsigned long max_lr =
         std::max(base_last_request_time, add_last_request_time);

       if (profile_match_params.change_last_request)
       {
         if (max_lr + session_timeout.tv_sec <
             static_cast<unsigned long>(current_time.tv_sec))
         {
           res_upw.session_start() = current_time.tv_sec;
         }
         else
         {
           res_upw.session_start() =
             std::max(base_session_start, add_session_start);
         }
       }
       else
       {
         res_upw.session_start() = match_to_add ? add_session_start : base_session_start;
       }

       try
       {
         if (!profile_match_params.no_match)
         {
           match_persistent_section_(
             result_channels,
             &res_upw.persistent_matches(),
             &upw.persistent_matches(),
             base_exists ? &base_persistent_matches : 0,
             add_exists ? &add_persistent_matches : 0,
             matched_channels.persistent_channels,
             match_to_add,
             profile_match_params.provide_persistent_channels);

           match_section_(
             result_channels,
             &res_upw.page_channels(),
             &upw.page_channels(),
             base_exists ? &base_page_channels : 0,
             add_exists ? &add_page_channels : 0,
             matched_channels.page_channels,
             channels.page_channels,
             current_time,
             match_to_add,
             res_upw.household() == 1);

           match_section_(
             result_channels,
             &res_upw.search_channels(),
             &upw.search_channels(),
             base_exists ? &base_search_channels : 0,
             add_exists ? &add_search_channels : 0,
             matched_channels.search_channels,
             channels.search_channels,
             current_time,
             match_to_add,
             res_upw.household() == 1);

           match_section_(
             result_channels,
             &res_upw.url_channels(),
             &upw.url_channels(),
             base_exists ? &base_url_channels : 0,
             add_exists ? &add_url_channels : 0,
             matched_channels.url_channels,
             channels.url_channels,
             current_time,
             match_to_add,
             res_upw.household() == 1);

           match_section_(
             result_channels,
             &res_upw.url_keyword_channels(),
             &upw.url_keyword_channels(),
             base_exists ? &base_url_keyword_channels : 0,
             add_exists ? &add_url_keyword_channels : 0,
             matched_channels.url_keyword_channels,
             channels.url_keyword_channels,
             current_time,
             match_to_add,
             res_upw.household() == 1);

           res_upw.audience_channels().reserve(base_audience_channels.size());
           for (ChannelsProfileReader::audience_channels_Container::const_iterator it =
                  base_audience_channels.begin();
                it != base_audience_channels.end(); ++it)
           {
             ChannelsHashMap::const_iterator c_it =
               channels.audience_channels.find((*it).channel_id());
             if (c_it != channels.audience_channels.end())
             {
               res_upw.audience_channels().push_back(*it);
             }
           }
         }
         else
         {
           if (match_to_add)
           {
             if (add_exists)
             {
               std::copy(
                 add_persistent_matches.channel_ids().begin(),
                 add_persistent_matches.channel_ids().end(),
                 std::back_inserter(res_upw.persistent_matches().channel_ids()));

               copy_section(res_upw.page_channels(), add_page_channels);
               copy_section(res_upw.search_channels(), add_search_channels);
               copy_section(res_upw.url_channels(), add_url_channels);
               copy_section(res_upw.url_keyword_channels(), add_url_keyword_channels);

               res_upw.last_page_triggers().clear();
               res_upw.last_page_triggers().reserve(add_rdr.last_page_triggers().size());
               std::copy(
                 add_rdr.last_page_triggers().begin(), add_rdr.last_page_triggers().end(),
                 std::back_inserter(res_upw.last_page_triggers()));

               res_upw.last_search_triggers().clear();
               res_upw.last_search_triggers().reserve(add_rdr.last_search_triggers().size());
               std::copy(
                 add_rdr.last_search_triggers().begin(), add_rdr.last_search_triggers().end(),
                 std::back_inserter(res_upw.last_search_triggers()));

               res_upw.last_url_triggers().clear();
               res_upw.last_url_triggers().reserve(add_rdr.last_url_triggers().size());
               std::copy(
                 add_rdr.last_url_triggers().begin(), add_rdr.last_url_triggers().end(),
                 std::back_inserter(res_upw.last_url_triggers()));

               res_upw.last_url_keyword_triggers().clear();
               res_upw.last_url_keyword_triggers().reserve(add_rdr.last_url_keyword_triggers().size());
               std::copy(
                 add_rdr.last_url_keyword_triggers().begin(),
                 add_rdr.last_url_keyword_triggers().end(),
                 std::back_inserter(res_upw.last_url_keyword_triggers()));

               res_upw.geo_data().clear();
               res_upw.geo_data().reserve(add_rdr.geo_data().size());
               std::copy(
                 add_rdr.geo_data().begin(), add_rdr.geo_data().end(),
                 std::back_inserter(res_upw.geo_data()));
             }
           }
           else
           {
             if (base_exists)
             {
               std::copy(
                 base_persistent_matches.channel_ids().begin(),
                 base_persistent_matches.channel_ids().end(),
                 std::back_inserter(res_upw.persistent_matches().channel_ids()));

               copy_section(res_upw.page_channels(), base_page_channels);
               copy_section(res_upw.search_channels(), base_search_channels);
               copy_section(res_upw.url_channels(), base_url_channels);
               copy_section(res_upw.url_keyword_channels(), base_url_keyword_channels);

               res_upw.last_page_triggers().clear();
               res_upw.last_page_triggers().reserve(base_rdr.last_page_triggers().size());
               std::copy(
                 base_rdr.last_page_triggers().begin(), base_rdr.last_page_triggers().end(),
                 std::back_inserter(res_upw.last_page_triggers()));

               res_upw.last_search_triggers().clear();
               res_upw.last_search_triggers().reserve(base_rdr.last_search_triggers().size());
               std::copy(
                 base_rdr.last_search_triggers().begin(), base_rdr.last_search_triggers().end(),
                 std::back_inserter(res_upw.last_search_triggers()));

               res_upw.last_url_triggers().clear();
               res_upw.last_url_triggers().reserve(base_rdr.last_url_triggers().size());
               std::copy(
                 base_rdr.last_url_triggers().begin(),
                 base_rdr.last_url_triggers().end(),
                 std::back_inserter(res_upw.last_url_triggers()));

               res_upw.last_url_keyword_triggers().clear();
               res_upw.last_url_keyword_triggers().reserve(base_rdr.last_url_keyword_triggers().size());
               std::copy(
                 base_rdr.last_url_keyword_triggers().begin(),
                 base_rdr.last_url_keyword_triggers().end(),
                 std::back_inserter(res_upw.last_url_keyword_triggers()));

               res_upw.geo_data().clear();
               res_upw.geo_data().reserve(base_rdr.geo_data().size());
               std::copy(
                 base_rdr.geo_data().begin(), base_rdr.geo_data().end(),
                 std::back_inserter(res_upw.geo_data()));

               upw.audience_channels().reserve(base_audience_channels.size());
               std::copy(
                 base_audience_channels.begin(),
                 base_audience_channels.end(),
                 std::back_inserter(upw.audience_channels()));
             }
           }

           result_channels.clear();
         }
       }
       catch(const eh::Exception& ex)
       {
         Stream::Error ostr;
         ostr << FUN << ": " << ex.what();
         throw InvalidProfileException(ostr);
       }

       if (!no_result_)
       {
         for (ChannelsProfileWriter::geo_data_Container::const_iterator it =
                res_upw.geo_data().begin(); it != res_upw.geo_data().end(); ++it)
         {
           GeoDataResult geo_res;
           geo_res.longitude = read_coord_((*it).longitude().data());
           geo_res.latitude = read_coord_((*it).latitude().data());
           geo_res.accuracy = read_accuracy_((*it).accuracy().data());

           properties.geo_data_list.push_back(geo_res);
         }
       }
       
       if (!match_to_add)
       {
         base_profile_->membuf().alloc(res_upw.size());
         res_upw.save(base_profile_->membuf().data(), res_upw.size());

         add_profile_->membuf().clear();
       }
       else
       {
         add_profile_->membuf().alloc(res_upw.size());
         res_upw.save(add_profile_->membuf().data(), res_upw.size());
       }

       if (no_result_)
       {
         result_channels.clear();
       }
     }

     void ChannelsMatcher::collect_channel_ids_(
       const Generics::MemBuf& base_profile,
       const ChannelDictionary& dictionary,
       AllUniqueChannels& auc)
       /*throw(Exception)*/
     {
       if (base_profile.size() != 0)
       {
         ChannelsProfileReader rdr(
           base_profile.data(),
           base_profile.size());

         fill_unique_channels_(rdr.page_channels(), dictionary, auc);
         fill_unique_channels_(rdr.search_channels(), dictionary, auc);
         fill_unique_channels_(rdr.url_channels(), dictionary, auc);
         fill_unique_channels_(rdr.url_keyword_channels(), dictionary, auc);
       }
     }

     void ChannelsMatcher::collect_history_channel_ids_(
       const Generics::MemBuf& history_profile,
       const ChannelDictionary& dictionary,
       AllUniqueChannels& auc)
       /*throw(Exception)*/
     {
       if (history_profile.size() != 0)
       {
         HistoryUserProfileReader rdr(
           history_profile.data(),
           history_profile.size());

         process_channels_sequence_(
           rdr.page_channels(), dictionary, HISTORY, auc);
         process_channels_sequence_(
           rdr.search_channels(), dictionary, HISTORY, auc);
         process_channels_sequence_(
           rdr.url_channels(), dictionary, HISTORY, auc);
         process_channels_sequence_(
           rdr.url_keyword_channels(), dictionary, HISTORY, auc);
       }
     }

     void ChannelsMatcher::fill_unique_channels_(
       const ChannelsInfoReader& section,
       const ChannelDictionary& dictionary,
       AllUniqueChannels& auc)
       /*throw(Exception)*/
     {
       process_channels_sequence_(
         section.ht_candidates(), dictionary, HISTORY, auc);
       process_channels_sequence_(
         section.history_matches(), dictionary, HISTORY, auc);
       process_channels_sequence_(
         section.history_visits(), dictionary, HISTORY, auc);
 //      process_channels_sequence_(
 //        section.session_matches(), dictionary, SESSION, auc);
     }

     template<typename ContainerType>
     void ChannelsMatcher::process_channels_sequence_(
       const ContainerType& sequence,
       const ChannelDictionary& dictionary,
       UniqueType channels_type,
       AllUniqueChannels& auc)
       /*throw(Exception)*/
     {
       typename ContainerType::const_iterator it = sequence.begin();

       const ChannelsHashMap& page = dictionary.page_channels;
       const ChannelsHashMap& search = dictionary.search_channels;
       const ChannelsHashMap& url = dictionary.url_channels;
       const ChannelsHashMap& url_keyword = dictionary.url_keyword_channels;

       if (channels_type == SESSION)
       {
         for (; it != sequence.end(); ++it)
         {
           if (auc.found_session_channels.find((*it).channel_id()) ==
               auc.found_session_channels.end())
           {
             ChannelsHashMap::const_iterator ch_it =
               page.find((*it).channel_id());

             if(ch_it == page.end())
             {
               ch_it = search.find((*it).channel_id());

               if (ch_it == search.end())
               {
                 ch_it = url.find((*it).channel_id());

                 if (ch_it == url.end())
                 {
                   ch_it = url_keyword.find((*it).channel_id());

                   if (ch_it == url_keyword.end())
                   {
                     continue;
                   }
                 }
               }
             }

             ChannelFeaturesMap::const_iterator f_it =
               dictionary.channel_features.find((*it).channel_id());

             if (f_it->second.discover)
             {
               auc.found_session_channels.insert((*it).channel_id());

               auc.discover.session_channels.insert((*it).channel_id());
               auc.discover.unique_channels.insert((*it).channel_id());
             }
             else
             {
               auc.found_session_channels.insert((*it).channel_id());

               auc.simple.session_channels.insert((*it).channel_id());
               auc.simple.unique_channels.insert((*it).channel_id());
             }
           }
         }
       }
       else
       {
         for (; it != sequence.end(); ++it)
         {
           if (auc.found_history_channels.find((*it).channel_id()) ==
               auc.found_history_channels.end())
           {
             ChannelsHashMap::const_iterator ch_it =
               page.find((*it).channel_id());

             if(ch_it == page.end())
             {
               ch_it = search.find((*it).channel_id());

               if (ch_it == search.end())
               {
                 ch_it = url.find((*it).channel_id());

                 if (ch_it == url.end())
                 {
                   ch_it = url_keyword.find((*it).channel_id());

                   if (ch_it == url_keyword.end())
                   {
                     continue;
                   }
                 }
               }
             }

             ChannelFeaturesMap::const_iterator f_it =
               dictionary.channel_features.find((*it).channel_id());

             if (f_it->second.discover)
             {
               auc.found_history_channels.insert((*it).channel_id());

               auc.discover.history_channels.insert((*it).channel_id());
               auc.discover.unique_channels.insert((*it).channel_id());
             }
             else
             {
               auc.found_history_channels.insert((*it).channel_id());

               auc.simple.history_channels.insert((*it).channel_id());
               auc.simple.unique_channels.insert((*it).channel_id());
             }
           }
         }
       }
     }

     template <typename BaseChannelsInfoType, typename AddChannelsInfoType>
     void ChannelsMatcher::merge_persistent_channels_(
       PersistentMatchesWriter& pmw,
       const BaseChannelsInfoType* base,
       const AddChannelsInfoType* add)
     {
       if (base != 0 && add != 0)
       {
         Algs::merge_unique(
           base->channel_ids().begin(), base->channel_ids().end(),
           add->channel_ids().begin(), add->channel_ids().end(),
           std::back_inserter(pmw.channel_ids()));
       }
       else if (base != 0)
       {
         std::copy(
           base->channel_ids().begin(), base->channel_ids().end(),
           std::back_inserter(pmw.channel_ids()));
       }
       else if (add != 0)
       {
         std::copy(
           add->channel_ids().begin(), add->channel_ids().end(),
           std::back_inserter(pmw.channel_ids()));
       }
     }

     void ChannelsMatcher::match_persistent_section_(
       ChannelMatchMap& result_channels,
       PersistentMatchesWriter* out_pmw,
       PersistentMatchesWriter* match_pmw,
       const PersistentMatchesReader* base_in,
       const PersistentMatchesReader* add_in,
       const ChannelIdVector& channels,
       bool match_to_add,
       bool provide_persistent_channels)
       /*throw(Exception)*/
     {
       try
       {
         for (ChannelIdVector::const_iterator it = channels.begin();
              it != channels.end(); ++it)
         {
           match_pmw->channel_ids().push_back(*it);
         }

         if ((match_to_add && add_in != 0) || (!match_to_add && base_in != 0))
         {
           Algs::merge_unique(
             match_pmw->channel_ids().begin(), match_pmw->channel_ids().end(),
             match_to_add ? (*add_in).channel_ids().begin() : (*base_in).channel_ids().begin(),
             match_to_add ? (*add_in).channel_ids().end() : (*base_in).channel_ids().end(),
             std::back_inserter((*out_pmw).channel_ids()));
         }
         else
         {
           std::copy(
             match_pmw->channel_ids().begin(), match_pmw->channel_ids().end(),
             std::back_inserter((*out_pmw).channel_ids()));
         }

         if(match_to_add && provide_persistent_channels)
         {
           PersistentMatchesWriter temp_pmw;

           merge_persistent_channels_(temp_pmw, base_in, out_pmw);

           for (PersistentMatchesWriter::channel_ids_Container::const_iterator it =
                  temp_pmw.channel_ids().begin();
                it != temp_pmw.channel_ids().end(); ++it)
           {
             add_weight_(result_channels, *it, 1);
           }
         }
         else if (provide_persistent_channels)
         {
           for (PersistentMatchesWriter::channel_ids_Container::const_iterator it =
                  out_pmw->channel_ids().begin();
                it != out_pmw->channel_ids().end(); ++it)
           {
             add_weight_(result_channels, *it, 1);
           }
         }
       }
       catch (const eh::Exception& ex)
       {
         Stream::Error ostr;
         ostr << "Attempt to match invalid profile: " << ex.what();        
         throw Exception(ostr);
       } 
     }

     void ChannelsMatcher::match_section_(
       ChannelMatchMap& result_channels,
       ChannelsInfoWriter* out_ciw,
       ChannelsInfoWriter* match_ciw,
       const ChannelsInfoReader* base_in,
       const ChannelsInfoReader* add_in,
       const ChannelIdVector& channels,
       const ChannelsHashMap& dictionary,
       const Generics::Time& now,
       bool match_to_add,
       bool household)
       /*throw(Exception)*/
     {
       try
       {
         match_ciw->history_visits().reserve(
           match_ciw->history_visits().size() + channels.size());
         match_ciw->session_matches().reserve(
           match_ciw->session_matches().size() + channels.size());

         for (ChannelIdVector::const_iterator it = channels.begin();
              it != channels.end(); ++it)
         {
           add_channel_visit_(now, *it, dictionary, *match_ciw, result_channels);
         }

         merge_channels_info_(
           *out_ciw,
           dictionary,
           now,
           match_to_add ? add_in : base_in,
           match_ciw,
           household);

         if(match_to_add)
         {
           ChannelsInfoWriter temp_ciw;

           merge_channels_info_(
             temp_ciw,
             dictionary,
             now,
             out_ciw,
             base_in,
             household);

           fill_channels_results_(
             result_channels,
             temp_ciw,
             now,
             dictionary);
         }
         else
         {
           fill_channels_results_(
             result_channels,
             *out_ciw,
             now,
             dictionary);
         }
       }
       catch (const eh::Exception& ex)
       {
         Stream::Error ostr;
         ostr << "Attempt to match invalid profile: " << ex.what();        
         throw Exception(ostr);
       }  
     }

     void ChannelsMatcher::fill_channels_results_(
       ChannelMatchMap& result_channels,
       const ChannelsInfoWriter& ciw,
       const Generics::Time& now,
       const ChannelsHashMap& channels)
     {
       for (ChannelsInfoWriter::session_matches_Container::
              const_iterator it = ciw.session_matches().begin();
            it != ciw.session_matches().end(); ++it)
       {
         ChannelsHashMap::const_iterator cit = channels.find((*it).channel_id());

         if (cit != channels.end())
         {
           /*
           for (ChannelIntervalList::const_iterator ci_it =
                  cit->second->short_intervals.begin();
                ci_it != cit->second->short_intervals.end(); ++ci_it)
           {
             unsigned long visits = ci_it->min_visits;

             for (WriteUIntVector::const_iterator dv_it = (*it).timestamps().begin();
                  dv_it != (*it).timestamps().end(); ++dv_it)
             {
               if (now.tv_sec - *dv_it >= ci_it->time_from.tv_sec &&
                   now.tv_sec - *dv_it <= ci_it->time_to.tv_sec)
               {
                 if(--visits == 0)
                 {
                   break;
                 }
               }
             }

             if (visits == 0)
             {
               add_weight_(result_channels, (*it).channel_id(), ci_it->weight);
             }
           }
           */

           ChannelIntervalList::const_iterator start_ci_it =
             cit->second->short_intervals.begin();
           ChannelIntervalList::const_iterator fin_ci_it =
             cit->second->short_intervals.end();
           ChannelIntervalList::const_iterator ci_it = start_ci_it;

           SessionMatchesWriter::timestamps_Container::
             const_iterator start_dv_it = (*it).timestamps().begin();
           SessionMatchesWriter::timestamps_Container::
             const_iterator fin_dv_it = (*it).timestamps().end();
           SessionMatchesWriter::timestamps_Container::const_iterator dv_it;

           while (ci_it != fin_ci_it)
           {
             unsigned long vis = ci_it->min_visits;
             dv_it = start_dv_it;

             while (dv_it != fin_dv_it)
             {
               if (now.tv_sec - *dv_it >= ci_it->time_from.tv_sec)
               {
                 if(now.tv_sec - *dv_it <= ci_it->time_to.tv_sec)
                 {
                   if (--vis == 0)
                   {
                     add_weight_(result_channels, (*it).channel_id(), ci_it->weight);
                     break;
                   }
                 }

                 ++dv_it;
               }
               else
               {
                 break;
               }
             }

             ++ci_it;
           }
         }
       }

       for (ChannelsInfoWriter::ht_candidates_Container::
              const_iterator it = ciw.ht_candidates().begin();
            it != ciw.ht_candidates().end(); ++it)
       {
         if ((*it).req_visits() == 0)
         {
           add_weight_(result_channels, (*it).channel_id(), (*it).weight());
         }
       }

       for (ChannelsInfoWriter::history_matches_Container::
              const_iterator it = ciw.history_matches().begin();
            it != ciw.history_matches().end(); ++it)
       {
         add_weight_(result_channels, (*it).channel_id(), (*it).weight());
       }
     }

     void ChannelsMatcher::add_weight_(
       ChannelMatchMap& result_channels,
       uint32_t index,
       uint32_t weight)
     {
       ChannelMatchMap::iterator it = result_channels.find(index);

       if (it != result_channels.end())
       {
         it->second += weight;
       }
       else if (!no_result_)
       {
         result_channels.insert(std::make_pair(index, weight));
       }
     }

     void ChannelsMatcher::merge(
       Generics::SmartMemBuf* history_profile,
       const Generics::MemBuf& other_base_profile,
       const Generics::MemBuf& other_history_profile,
       const ChannelDictionary& channels,
       const ProfileMatchParams& match_params,
       const Generics::Time& merge_time)
       /*throw(InvalidProfileException)*/
     {
       static const char* FUN = "ChannelsMatcher::merge()";

       try
       {
         bool base_exists = false;
         bool other_base_exists = false;

         ChannelsProfileReader base_rdr(0, 0);

         uint32_t base_last_request_time = 0;
         uint32_t base_household = 0;
         uint32_t base_ignore_fraud_time = 0;
         uint32_t base_create_time = merge_time.tv_sec;
         uint32_t base_history_time = merge_time.tv_sec;
         uint32_t base_first_colo_id = match_params.request_colo_id;
         uint32_t base_last_colo_id = match_params.request_colo_id;
         std::string base_cohort;

         if (base_profile_->membuf().size() != 0)
         {
           base_exists = true;

           ChannelsProfileReader temp_base_rdr(
             base_profile_->membuf().data(),
             base_profile_->membuf().size());

           base_last_request_time = temp_base_rdr.last_request_time();
           base_household = temp_base_rdr.household();
           base_ignore_fraud_time = temp_base_rdr.ignore_fraud_time();
           base_create_time = temp_base_rdr.create_time();
           base_history_time = temp_base_rdr.history_time();

           base_first_colo_id = temp_base_rdr.first_colo_id();
           base_last_colo_id = temp_base_rdr.last_colo_id();

           base_rdr = temp_base_rdr;

           base_cohort = temp_base_rdr.cohort();
         }

         ChannelsProfileReader other_base_rdr(0, 0);

         uint32_t other_base_history_time = merge_time.tv_sec;
         uint32_t other_base_ignore_fraud_time = 0;
         if (other_base_profile.size() != 0)
         {
           other_base_exists = true;

           ChannelsProfileReader temp_base_rdr(
             other_base_profile.data(),
             other_base_profile.size());

           other_base_history_time = temp_base_rdr.history_time();
           other_base_ignore_fraud_time = temp_base_rdr.ignore_fraud_time();

           other_base_rdr = temp_base_rdr;
         }

 /*        
         Generics::MemBuf base = base_profile_->membuf();
         if (base.empty())
         {
           ChannelsProfileWriter profile_writer;
           profile_writer.version() = CURRENT_BASE_PROFILE_VERSION;
           profile_writer.create_time() = merge_time.integer_part();
           profile_writer.history_time() = merge_time.integer_part();
           profile_writer.ignore_fraud_time() = 0;
           profile_writer.last_request_time() = 0;
           profile_writer.session_start() = 0;

           base.alloc(profile_writer.size());
           profile_writer.save(base.data(), profile_writer.size());
         }


         Generics::MemBuf other_base = other_base_profile;
         if (other_base.empty())
         {
           ChannelsProfileWriter empty_base_profile_writer;
           empty_base_profile_writer.version() = CURRENT_BASE_PROFILE_VERSION;
           empty_base_profile_writer.create_time() = merge_time.integer_part();
           empty_base_profile_writer.history_time() = merge_time.integer_part();
           empty_base_profile_writer.ignore_fraud_time() = 0;
           empty_base_profile_writer.last_request_time() = 0;
           empty_base_profile_writer.session_start() = 0;

           other_base.alloc(empty_base_profile_writer.size());
           empty_base_profile_writer.save(
             other_base.data(), empty_base_profile_writer.size());
         }
 */
         ChannelsProfileWriter result_profile_writer; 
         result_profile_writer.version() = CURRENT_BASE_PROFILE_VERSION;
         result_profile_writer.create_time() = base_create_time;
         result_profile_writer.history_time() =
           std::min(base_history_time, other_base_history_time);
         result_profile_writer.ignore_fraud_time() =
           std::max(base_ignore_fraud_time, other_base_ignore_fraud_time);
         result_profile_writer.last_request_time() = 0;
         result_profile_writer.session_start() = 0;
         result_profile_writer.household() = match_params.household || base_household;
         result_profile_writer.first_colo_id() =
           (base_first_colo_id != UNKNOWN_COLO_ID && base_first_colo_id != DEFAULT_COLO) ?
           base_first_colo_id : match_params.request_colo_id;
         result_profile_writer.last_colo_id() =
           base_last_colo_id != UNKNOWN_COLO_ID ?
             base_last_colo_id : match_params.request_colo_id;
         result_profile_writer.cohort() = base_cohort;

         merge_(
           result_profile_writer,
           channels,
           Generics::Time(base_last_request_time),
           base_exists ? &base_rdr : 0,
           other_base_exists ? &other_base_rdr : 0);

         result_profile_writer.last_request_time() =
           match_params.change_last_request ?
             merge_time.tv_sec : base_last_request_time;

         base_profile_->membuf().alloc(result_profile_writer.size());
         result_profile_writer.save(
           base_profile_->membuf().data(), result_profile_writer.size());

         HistoryUserProfileWriter hupw;
         hupw.minor_version() = CURRENT_HISTORY_MINOR_PROFILE_VERSION;
         hupw.major_version() = CURRENT_HISTORY_MAJOR_PROFILE_VERSION;

         HistoryUserProfileReader::page_channels_Container base_page_channels;
         HistoryUserProfileReader::search_channels_Container base_search_channels;
         HistoryUserProfileReader::url_channels_Container base_url_channels;
         HistoryUserProfileReader::url_keyword_channels_Container
           base_url_keyword_channels;

         if(history_profile && !history_profile->membuf().empty())
         {
           HistoryUserProfileReader base_hpr(
             history_profile->membuf().data(),
             history_profile->membuf().size());

           base_page_channels = base_hpr.page_channels();
           base_search_channels = base_hpr.search_channels();
           base_url_channels = base_hpr.url_channels();
           base_url_keyword_channels = base_hpr.url_keyword_channels();
         }

         HistoryUserProfileReader::page_channels_Container other_page_channels;
         HistoryUserProfileReader::search_channels_Container other_search_channels;
         HistoryUserProfileReader::url_channels_Container other_url_channels;
         HistoryUserProfileReader::url_keyword_channels_Container
           other_url_keyword_channels;

         if (!other_history_profile.empty())
         {
           HistoryUserProfileReader other_hpr(
             other_history_profile.data(),
             other_history_profile.size());

           other_page_channels = other_hpr.page_channels();
           other_search_channels = other_hpr.search_channels();
           other_url_channels = other_hpr.url_channels();
           other_url_keyword_channels = other_hpr.url_keyword_channels();
         }

         Generics::Time last_request_time(base_last_request_time);

         merge_history_data_(
           hupw.search_channels(),
           base_search_channels,
           other_search_channels,
           last_request_time,
           channels.search_channels);

         merge_history_data_(
           hupw.url_channels(),
           base_url_channels,
           other_url_channels,
           last_request_time,
           channels.url_channels);

         merge_history_data_(
           hupw.page_channels(),
           base_page_channels,
           other_page_channels,
           last_request_time,
           channels.page_channels);

         merge_history_data_(
           hupw.url_keyword_channels(),
           base_url_keyword_channels,
           other_url_keyword_channels,
           last_request_time,
           channels.url_keyword_channels);

         if (history_profile &&
             (!history_profile->membuf().empty() || !other_history_profile.empty()))
         {
           history_profile->membuf().alloc(hupw.size());
           hupw.save(history_profile->membuf().data(), hupw.size());
         }
       }
       catch (const eh::Exception& ex)
       {
         Stream::Error ostr;
         ostr << FUN << ": Attempt to merge invalid profiles: " << ex.what();        
         throw InvalidProfileException(ostr);
       }  
     }

     bool ChannelsMatcher::need_history_optimization(
       const Generics::Time& now,
       const Generics::Time& period,
       const Generics::Time& gmt_offset)
       const
       /*throw(InvalidProfileException)*/
     {
       static const char* FUN = "ChannelsMatcher::need_history_optimization()";

       try
       {
         if (!base_profile_->membuf().empty() && add_profile_->membuf().empty())
         {
           return need_history_optimization_(
             base_profile_.in(),
             now,
             period,
             gmt_offset);
         }
       }
       catch (const eh::Exception& ex)
       {
         Stream::Error ostr;
         ostr << FUN << ": Attempt to match invalid profile: " << ex.what();
         throw InvalidProfileException(ostr);
       }

       return false;
     }

     template <typename HistoryChannelsInfoListWriter,
               typename HistoryChannelsInfoListReader>
     void ChannelsMatcher::history_optimize_(
       ChannelsInfoWriter& channels_info_writer,
       HistoryChannelsInfoListWriter& history_channels_writer,
       const HistoryChannelsInfoListReader& history_channels_reader,
       const ChannelsInfoReader& channels_info_reader,
       const ChannelsHashMap& channels,
       unsigned long days,
       const Generics::Time& now,
       bool /*household*/)
     {
       // copy and clean session matches
       channels_info_writer.session_matches().reserve(
         channels_info_writer.session_matches().size() + channels_info_reader.session_matches().size());

       std::copy(
         channels_info_reader.session_matches().begin(),
         channels_info_reader.session_matches().end(),
         Algs::modify_inserter(
           Algs::filter_inserter(
             std::back_inserter(channels_info_writer.session_matches()),
             SessionMatchesFilter()),
           SessionMatchesCleaner(channels, now)));

       /* merge HistoryVisits and HistoryProfile
        * with shift history information for days and
        * save into new history profile */

       if (!history_channels_reader.empty())
       {
         history_channels_writer.reserve(
           history_channels_reader.size() + channels_info_reader.history_visits().size());

         Algs::merge_unique(
           history_channels_reader.begin(),
           history_channels_reader.end(),
           channels_info_reader.history_visits().begin(),
           channels_info_reader.history_visits().end(),
           Algs::modify_inserter(
             Algs::modify_inserter( // clean excess visits
               Algs::filter_inserter( // filter empty channels
                 std::back_inserter(history_channels_writer),
                 HistoryChannelInfoFilter(channels)),
               HistoryChannelInfoCleaner(channels)),
             HistoryChannelInfoConverter(days)),
           ChannelIdLess(),
           HistoryChannelInfoHistoryVisitsMerge(days));
       }
       else
       {
         history_channels_writer.reserve(channels_info_reader.history_visits().size());

         std::copy(
           channels_info_reader.history_visits().begin(),
           channels_info_reader.history_visits().end(),
           Algs::modify_inserter(
             Algs::modify_inserter(
               Algs::filter_inserter(
                 std::back_inserter(history_channels_writer),
                 HistoryChannelInfoFilter(channels)),
               HistoryChannelInfoCleaner(channels)),
             HistoryChannelInfoConverter(days)));
       }

       /* fill HistoryTodayCandidates by HistoryProfile and
        * partly match result only for History(Long) channels */
       fill_history_candidates_(
         channels_info_writer,
         history_channels_writer,
         channels);
     }

     void ChannelsMatcher::history_optimize(
       Generics::SmartMemBuf* history_profile,
       const Generics::Time& now,
       const Generics::Time& gmt_offset,
       const ChannelDictionary& channels,
       bool* first_today_history_optimization)
     {
       ChannelsProfileReader rdr(
         base_profile_->membuf().data(), 
         base_profile_->membuf().size());

       Generics::Time current_time(
         std::max(static_cast<uint32_t>(now.tv_sec),
                  rdr.last_request_time()));

       HistoryUserProfileReader::page_channels_Container hrdr_page;
       HistoryUserProfileReader::page_channels_Container hrdr_search;
       HistoryUserProfileReader::page_channels_Container hrdr_url;
       HistoryUserProfileReader::page_channels_Container hrdr_url_keyword;

       if(!history_profile->membuf().empty())
       {
         HistoryUserProfileReader hrdr(
           history_profile->membuf().data(),
           history_profile->membuf().size());

         hrdr_page = hrdr.page_channels();
         hrdr_search = hrdr.search_channels();
         hrdr_url = hrdr.url_channels();
         hrdr_url_keyword = hrdr.url_keyword_channels();
       }

       HistoryUserProfileWriter hwr;
       hwr.minor_version() = CURRENT_HISTORY_MINOR_PROFILE_VERSION;
       hwr.major_version() = CURRENT_HISTORY_MAJOR_PROFILE_VERSION;

       uint32_t days =
         (current_time.tv_sec + gmt_offset.tv_sec)/SEC_IN_DAY -
         (rdr.history_time() + gmt_offset.tv_sec)/SEC_IN_DAY;

       if (first_today_history_optimization != 0)
       {
         *first_today_history_optimization = days != 0;
       }

       ChannelsProfileWriter bwr;
       bwr.version() = CURRENT_BASE_PROFILE_VERSION;
       bwr.last_request_time() = rdr.last_request_time();
       bwr.history_time() = current_time.tv_sec;
       bwr.create_time() = rdr.create_time();
       bwr.ignore_fraud_time() = rdr.ignore_fraud_time();
       bwr.session_start() = rdr.session_start();
       bwr.household() = rdr.household();
       bwr.first_colo_id() = rdr.first_colo_id();
       bwr.last_colo_id() = rdr.last_colo_id();
       bwr.cohort() = rdr.cohort();

       std::copy(
         rdr.persistent_matches().channel_ids().begin(),
         rdr.persistent_matches().channel_ids().end(),
         std::back_inserter(bwr.persistent_matches().channel_ids()));

       bwr.last_page_triggers().reserve(
         bwr.last_page_triggers().size() + rdr.last_page_triggers().size());
       std::copy(
         rdr.last_page_triggers().begin(),
         rdr.last_page_triggers().end(),
         std::back_inserter(bwr.last_page_triggers()));

       bwr.last_search_triggers().reserve(
         bwr.last_search_triggers().size() + rdr.last_search_triggers().size());
       std::copy(
         rdr.last_search_triggers().begin(),
         rdr.last_search_triggers().end(),
         std::back_inserter(bwr.last_search_triggers()));

       bwr.last_url_triggers().reserve(
         bwr.last_url_triggers().size() + rdr.last_url_triggers().size());
       std::copy(
         rdr.last_url_triggers().begin(),
         rdr.last_url_triggers().end(),
         std::back_inserter(bwr.last_url_triggers()));

       bwr.last_url_keyword_triggers().reserve(
         bwr.last_url_keyword_triggers().size() + rdr.last_url_keyword_triggers().size());
       std::copy(
         rdr.last_url_keyword_triggers().begin(),
         rdr.last_url_keyword_triggers().end(),
         std::back_inserter(bwr.last_url_keyword_triggers()));

       bwr.geo_data().reserve(bwr.geo_data().size() + rdr.geo_data().size());
       std::copy(
         rdr.geo_data().begin(),
         rdr.geo_data().end(),
         std::back_inserter(bwr.geo_data()));

       history_optimize_(
         bwr.page_channels(),
         hwr.page_channels(),
         hrdr_page,
         rdr.page_channels(),
         channels.page_channels,
         days,
         current_time,
         rdr.household() == 1);

       history_optimize_(
         bwr.search_channels(),
         hwr.search_channels(),
         hrdr_search,
         rdr.search_channels(),
         channels.search_channels,
         days,
         current_time,
         rdr.household() == 1);

       history_optimize_(
         bwr.url_channels(),
         hwr.url_channels(),
         hrdr_url,
         rdr.url_channels(),
         channels.url_channels,
         days,
         current_time,
         rdr.household() == 1);

       history_optimize_(
         bwr.url_keyword_channels(),
         hwr.url_keyword_channels(),
         hrdr_url_keyword,
         rdr.url_keyword_channels(),
         channels.url_keyword_channels,
         days,
         current_time,
         rdr.household() == 1);

      bwr.audience_channels().reserve(bwr.audience_channels().size() + rdr.audience_channels().size());

      for (ChannelsProfileReader::audience_channels_Container::const_iterator it =
            rdr.audience_channels().begin();
          it != rdr.audience_channels().end(); ++it)
      {
        ChannelsHashMap::const_iterator c_it =
          channels.audience_channels.find((*it).channel_id());
        if (c_it != channels.audience_channels.end())
        {
          if (current_time.tv_sec / SEC_IN_DAY - (*it).time() / SEC_IN_DAY <
              (c_it->second->today_long_intervals.begin())->time_to.tv_sec / SEC_IN_DAY)
          {
            bwr.audience_channels().push_back(*it);
          }
        }
      }

      unsigned long sz = hwr.size();
      history_profile->membuf().alloc(sz);
      hwr.save(history_profile->membuf().data(), sz);

      sz = bwr.size();
      base_profile_->membuf().alloc(sz);
      bwr.save(base_profile_->membuf().data(), sz);
    }

    template <typename HistoryChannelInfoList>
    void ChannelsMatcher::fill_history_candidates_(
      ChannelsInfoWriter& ciw,
      const HistoryChannelInfoList& hciw,
      const ChannelsHashMap& channels)
    {
      ciw.ht_candidates().reserve(ciw.ht_candidates().size() + hciw.size());
      ciw.history_matches().reserve(ciw.history_matches().size() + hciw.size());

      for (typename HistoryChannelInfoList::const_iterator it = hciw.begin();
           it != hciw.end(); ++it)
      {
        unsigned long weight = history_matched_(*it, channels);
        
        if (weight != 0)
        {
          HistoryMatchesWriter hmw;
          hmw.channel_id() = (*it).channel_id();
          hmw.weight() = weight;
          ciw.history_matches().push_back(hmw);
        }

        fill_history_candidate_visits_(
          ciw.ht_candidates(),
          *it,
          channels);
      }
    }

    bool ChannelsMatcher::need_history_optimization_(
      const Generics::SmartMemBuf* profile,
      const Generics::Time& now,
      const Generics::Time& period,
      const Generics::Time& gmt_offset)
      const
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::need_history_optimization_()";
      
      try
      {
        if (!profile->membuf().empty())
        {
          ChannelsProfileReader rdr(profile->membuf().data(), profile->membuf().size());

          if (rdr.history_time() != 0 &&
              ((now + gmt_offset).get_gm_time().get_date() >
               (Generics::Time(
                  rdr.history_time()) + gmt_offset).get_gm_time().get_date() ||
               (period != Generics::Time::ZERO &&
                now.get_gm_time() >
                (period + Generics::Time(rdr.history_time())))))
          {
            return true;
          }
        }
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Attempt to match invalid profile: " << ex.what();
        throw InvalidProfileException(ostr);
      }  

      return false;
    }

    unsigned long ChannelsMatcher::history_matched_(
      const HistoryChannelInfoWriter& hciw,
      const ChannelsHashMap& channels)
    {
      ChannelsHashMap::const_iterator cit = channels.find(hciw.channel_id());

      if (cit == channels.end())
      {
        return 0;
      }

      unsigned long res = 0;

      for (ChannelIntervalList::const_iterator it =
             cit->second->long_intervals.begin();
           it != cit->second->long_intervals.end(); ++it)
      {
        unsigned long interval_visits = 0;
//        unsigned long from_now_visits = 0;

        for (HistoryChannelInfoWriter::days_visits_Container::const_iterator d_it =
               hciw.days_visits().begin();
             d_it != hciw.days_visits().end(); ++d_it)
        {
          if((*d_it).days() < it->time_to.tv_sec / SEC_IN_DAY)
          {
            if ((*d_it).days() >= it->time_from.tv_sec / SEC_IN_DAY)
            {
              interval_visits += (*d_it).visits();
            }

//            from_now_visits += (*d_it).visits();
          }
        }

        if (interval_visits >= it->min_visits)
        {
          res += it->weight;
        }
      }

      /*
      for (ChannelIntervalList::const_iterator it =
             cit->second->today_long_intervals.begin();
           it != cit->second->today_long_intervals.end(); ++it)
      {
        unsigned long interval_visits = 0;

        for (WriteDVVector::const_iterator d_it = hciw.days_visits().begin();
             d_it != hciw.days_visits().end(); ++d_it)
        {
          if ((*d_it).days() >= it->time_from.tv_sec/SEC_IN_DAY &&
              (*d_it).days() < it->time_to.tv_sec/SEC_IN_DAY)
          {
            interval_visits += (*d_it).visits();
          }
        }

        if (interval_visits >= it->min_visits)
        {
          res += it->weight;
        }
      }
      */
      
      return res;
    }

    void ChannelsMatcher::fill_history_candidate_visits_(
      ChannelsInfoWriter::ht_candidates_Container& ht_candidates,
      const HistoryChannelInfoWriter& hciw,
      const ChannelsHashMap& channels)
    {
      ChannelsHashMap::const_iterator cit = channels.find(hciw.channel_id());

      if (cit == channels.end())
      {
        return;
      }

      bool insert_all_ints = false;

      for (ChannelIntervalList::const_iterator it =
             cit->second->today_long_intervals.begin();
           it != cit->second->today_long_intervals.end(); ++it)
      {
        unsigned long found_visits = 0;
        
        for (HistoryChannelInfoWriter::days_visits_Container::const_iterator d_it =
               hciw.days_visits().begin();
             d_it != hciw.days_visits().end(); ++d_it)
        {
          if ((*d_it).days() >= it->time_from.tv_sec/SEC_IN_DAY &&
              (*d_it).days() < it->time_to.tv_sec/SEC_IN_DAY)
          {
            found_visits += (*d_it).visits();
          }
        }

        if(found_visits != 0 || insert_all_ints)
        {
          if(!insert_all_ints)
          {
            for (ChannelIntervalList::const_iterator lit =
              cit->second->today_long_intervals.begin();
              lit != it; ++lit)
            {
              HTCandidatesWriter new_ht_candidate;
              new_ht_candidate.channel_id() = hciw.channel_id();
              new_ht_candidate.req_visits() = lit->min_visits;
              new_ht_candidate.visits() = 0;
              new_ht_candidate.weight() = 0;
              ht_candidates.push_back(new_ht_candidate);
            }
          }

          insert_all_ints = true;

          HTCandidatesWriter new_ht_candidate;
          new_ht_candidate.channel_id() = hciw.channel_id();
          new_ht_candidate.req_visits() =
            found_visits < it->min_visits ? it->min_visits - found_visits : 0;
          new_ht_candidate.visits() = found_visits;
          new_ht_candidate.weight() =
            new_ht_candidate.req_visits() != 0 ? 0 : it->weight;

          ht_candidates.push_back(new_ht_candidate);
        }
      }
    }

    void ChannelsMatcher::add_channel_visit_(
      const Generics::Time& now,
      const unsigned long channel_id,
      const ChannelsHashMap& channels,
      ChannelsInfoWriter& ciw,
      ChannelMatchMap& result_channels) noexcept
    {
      ChannelsHashMap::const_iterator c_it = channels.find(channel_id);

      if (c_it != channels.end())
      {
        if (c_it->second->zero_channel)
        {
          add_weight_(result_channels, channel_id, c_it->second->weight);
        }
         
        if (!c_it->second->short_intervals.empty())
        {
          SessionMatchesWriter smw;
          smw.channel_id() = channel_id;
          smw.timestamps().push_back(now.tv_sec);

          ciw.session_matches().push_back(smw);
        }
  
        if (!c_it->second->long_intervals.empty() ||
            !c_it->second->today_long_intervals.empty())
        {
          HistoryVisitsWriter hvw;
          hvw.channel_id() = channel_id;
          hvw.visits() = 1;
          ciw.history_visits().push_back(hvw);
        }

        if (!c_it->second->today_long_intervals.empty())
        {
          const ChannelIntervalList& tli_list = c_it->second->today_long_intervals;
          ciw.ht_candidates().reserve(ciw.ht_candidates().size() + tli_list.size());

          for (ChannelIntervalList::const_iterator ci_it = tli_list.begin(); 
               ci_it != tli_list.end(); ++ci_it)
          {
            HTCandidatesWriter htcw;
            htcw.channel_id() = channel_id;
            htcw.req_visits() = ci_it->min_visits - 1;
            htcw.visits() = 1;
            htcw.weight() = htcw.req_visits() == 0 ? ci_it->weight : 0;
            
            ciw.ht_candidates().push_back(htcw);
          }
        }
      }
    }

    template <typename HistoryChannelInfoListWriter,
              typename HistoryChannelInfoListReader>
    void ChannelsMatcher::merge_history_data_(
      HistoryChannelInfoListWriter& hw,
      const HistoryChannelInfoListReader& base,
      const HistoryChannelInfoListReader& add,
      const Generics::Time& /* now */,
      const ChannelsHashMap& channels)
    {
      hw.reserve(base.size() + add.size());

      Algs::merge_unique(
        base.begin(), base.end(),
        add.begin(), add.end(),
        Algs::filter_inserter(
          std::back_inserter(hw),
          HistoryChannelInfoFilter(channels)),
        ChannelIdLess(),
        HistoryChannelInfoMerge());
    }

    template <typename BaseProfileType, typename AddProfileType>
    void ChannelsMatcher::merge_(
      ChannelsProfileWriter& upw,
      const ChannelDictionary& channels,
      const Generics::Time& now,
      const BaseProfileType* base,
      const AddProfileType* add)
      /*throw(InvalidProfileException)*/
    {
      static const char* FUN = "ChannelsMatcher::merge_<>()";
      
      try
      {
        bool base_exists = base != 0;
        bool add_exists = add != 0;
        
        upw.last_request_time() = std::max(
          base ? base->last_request_time() : 0,
          add ? add->last_request_time() : 0);

        upw.session_start() = base ? base->session_start() : 0;

        ChannelsInfoReader base_page_channels(0, 0);
        ChannelsInfoReader base_search_channels(0, 0);
        ChannelsInfoReader base_url_channels(0, 0);
        ChannelsInfoReader base_url_keyword_channels(0, 0);
        ChannelsProfileReader::audience_channels_Container base_audience_channels;
        
        PersistentMatchesReader base_persistent_channels(0, 0);

        ChannelsProfileReader::last_page_triggers_Container base_last_page_triggers;
        ChannelsProfileReader::last_search_triggers_Container base_last_search_triggers;
        ChannelsProfileReader::last_url_triggers_Container base_last_url_triggers;
        ChannelsProfileReader::last_url_keyword_triggers_Container
          base_last_url_keyword_triggers;

        ChannelsProfileReader::geo_data_Container base_geo_data;

        if (base_exists)
        {
          base_page_channels = base->page_channels();
          base_search_channels = base->search_channels();
          base_url_channels = base->url_channels();
          base_url_keyword_channels = base->url_keyword_channels();
          
          base_audience_channels = base->audience_channels();
          
          base_persistent_channels = base->persistent_matches();

          base_last_page_triggers = base->last_page_triggers();
          base_last_search_triggers = base->last_search_triggers();
          base_last_url_triggers = base->last_url_triggers();
          base_last_url_keyword_triggers = base->last_url_keyword_triggers();

          base_geo_data = base->geo_data();
        }

        ChannelsInfoReader add_page_channels(0, 0);
        ChannelsInfoReader add_search_channels(0, 0);
        ChannelsInfoReader add_url_channels(0, 0);
        ChannelsInfoReader add_url_keyword_channels(0, 0);
        PersistentMatchesReader add_persistent_channels(0, 0);

        ChannelsProfileReader::last_page_triggers_Container add_last_page_triggers;
        ChannelsProfileReader::last_search_triggers_Container add_last_search_triggers;
        ChannelsProfileReader::last_url_triggers_Container add_last_url_triggers;
        ChannelsProfileReader::last_url_keyword_triggers_Container
          add_last_url_keyword_triggers;

        ChannelsProfileReader::geo_data_Container add_geo_data;

        if (add_exists)
        {
          add_page_channels = add->page_channels();
          add_search_channels = add->search_channels();
          add_url_channels = add->url_channels();
          add_url_keyword_channels = add->url_keyword_channels();

          add_persistent_channels = add->persistent_matches();

          add_last_page_triggers = add->last_page_triggers();
          add_last_search_triggers = add->last_search_triggers();
          add_last_url_triggers = add->last_url_triggers();
          add_last_url_keyword_triggers = add->last_url_keyword_triggers();

          add_geo_data = add->geo_data();
        }

        if (base_exists)
        {
          upw.audience_channels().reserve(
            upw.audience_channels().size() + base_audience_channels.size());

          std::copy(
            base_audience_channels.begin(),
            base_audience_channels.end(),
            std::back_inserter(upw.audience_channels()));
        }
        
        merge_persistent_channels_(
          upw.persistent_matches(),
          base_exists ? &base_persistent_channels : 0,
          add_exists ? &add_persistent_channels : 0);
        
        merge_channels_info_(
          upw.page_channels(), 
          channels.page_channels, 
          now,
          base_exists ? &base_page_channels : 0,
          add_exists ? &add_page_channels : 0);

        merge_channels_info_(
          upw.url_channels(), 
          channels.url_channels, 
          now,
          base_exists ? &base_url_channels : 0,
          add_exists ? &add_url_channels : 0);

        merge_channels_info_(
          upw.url_keyword_channels(), 
          channels.url_keyword_channels, 
          now,
          base_exists ? &base_url_keyword_channels : 0,
          add_exists ? &add_url_keyword_channels : 0);

        merge_channels_info_(
          upw.search_channels(), 
          channels.search_channels, 
          now,
          base_exists ? &base_search_channels : 0,
          add_exists ? &add_search_channels : 0);

        upw.last_page_triggers().reserve(
          upw.last_page_triggers().size() + add_last_page_triggers.size() +
          base_last_page_triggers.size());
        Algs::merge_unique(
          base_last_page_triggers.begin(), base_last_page_triggers.end(),
          add_last_page_triggers.begin(), add_last_page_triggers.end(),
          Algs::filter_inserter(
            std::back_inserter(upw.last_page_triggers()),
            ExistChannelFilter(channels.page_channels)),
          ChannelMatchLess(),
          LastTriggersMerge());

        upw.last_search_triggers().reserve(
          upw.last_search_triggers().size() + add_last_search_triggers.size() +
          base_last_search_triggers.size());
        Algs::merge_unique(
          base_last_search_triggers.begin(), base_last_search_triggers.end(),
          add_last_search_triggers.begin(), add_last_search_triggers.end(),
          Algs::filter_inserter(
            std::back_inserter(upw.last_search_triggers()),
            ExistChannelFilter(channels.search_channels)),
          ChannelMatchLess(),
          LastTriggersMerge());

        upw.last_url_triggers().reserve(
          upw.last_url_triggers().size() + add_last_url_triggers.size() +
          base_last_url_triggers.size());
        Algs::merge_unique(
          base_last_url_triggers.begin(), base_last_url_triggers.end(),
          add_last_url_triggers.begin(), add_last_url_triggers.end(),
          Algs::filter_inserter(
            std::back_inserter(upw.last_url_triggers()),
            ExistChannelFilter(channels.url_channels)),
          ChannelMatchLess(),
          LastTriggersMerge());

        upw.last_url_keyword_triggers().reserve(
          upw.last_url_keyword_triggers().size() + add_last_url_keyword_triggers.size() +
          base_last_url_keyword_triggers.size());
        Algs::merge_unique(
          base_last_url_keyword_triggers.begin(), base_last_url_keyword_triggers.end(),
          add_last_url_keyword_triggers.begin(), add_last_url_keyword_triggers.end(),
          Algs::filter_inserter(
            std::back_inserter(upw.last_url_keyword_triggers()),
            ExistChannelFilter(channels.url_keyword_channels)),
          ChannelMatchLess(),
          LastTriggersMerge());

        /*
        merge_geo_data_(
          upw.geo_data(),
          base_geo_data,
          add_geo_data,
          now);
        */
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception: " << ex.what();
        throw InvalidProfileException(ostr);
      }
    }

    template <typename BaseChannelsInfoType, typename AddChannelsInfoType>
    void ChannelsMatcher::merge_channels_info_(
      ChannelsInfoWriter& ciw,
      const ChannelsHashMap& channels,
      const Generics::Time& now,
      const BaseChannelsInfoType* base,
      const AddChannelsInfoType* add,
      bool /*household*/)
    {
      if (base && add)
      {
        ciw.ht_candidates().reserve(
          ciw.ht_candidates().size() + base->ht_candidates().size() + add->ht_candidates().size());

        merge_htc_(
          ciw.ht_candidates(),
          channels,
          base->ht_candidates(),
          add->ht_candidates());

        ciw.history_matches().reserve(
          ciw.history_matches().size() + base->history_matches().size() + add->history_matches().size());

        Algs::merge_unique(
          base->history_matches().begin(), base->history_matches().end(),
          add->history_matches().begin(), add->history_matches().end(),
          Algs::filter_inserter(
            std::back_inserter(ciw.history_matches()),
            ExistChannelFilter(channels)),
          ChannelIdLess(),
          Algs::FirstArg());

        ciw.history_visits().reserve(
          ciw.history_visits().size() + base->history_visits().size() + add->history_visits().size());

        Algs::merge_unique(
          base->history_visits().begin(), base->history_visits().end(),
          add->history_visits().begin(), add->history_visits().end(),
          Algs::filter_inserter(
            std::back_inserter(ciw.history_visits()),
            ExistChannelFilter(channels)),
          ChannelIdLess(),
          HistoryVisitsMerge());

        ciw.session_matches().reserve(
          ciw.session_matches().size() + base->session_matches().size() + add->session_matches().size());

        Algs::merge_unique(
          base->session_matches().begin(), base->session_matches().end(),
          add->session_matches().begin(), add->session_matches().end(),
          Algs::modify_inserter(
            Algs::filter_inserter(
              std::back_inserter(ciw.session_matches()),
              SessionMatchesFilter()),
            SessionMatchesCleaner(channels, now)),
          ChannelIdLess(),
          SessionMatchesMerge());
      }
      else if (base)
      {
        ChannelsInfoReader::ht_candidates_Container empty_ht;
        ciw.ht_candidates().reserve(ciw.ht_candidates().size() + base->ht_candidates().size());

        merge_htc_(
          ciw.ht_candidates(),
          channels,
          base->ht_candidates(),
          empty_ht);

        ciw.history_matches().reserve(
          ciw.history_matches().size() + base->history_matches().size());

        std::copy(
          base->history_matches().begin(), base->history_matches().end(),
          Algs::filter_inserter(
            std::back_inserter(ciw.history_matches()),
            ExistChannelFilter(channels)));

        ciw.history_visits().reserve(ciw.history_visits().size() + base->history_visits().size());

        std::copy(
          base->history_visits().begin(), base->history_visits().end(),
          Algs::filter_inserter(
            std::back_inserter(ciw.history_visits()),
            ExistChannelFilter(channels)));

        ciw.session_matches().reserve(ciw.session_matches().size() + base->session_matches().size());

        std::copy(
          base->session_matches().begin(), base->session_matches().end(),
          Algs::modify_inserter(
            Algs::filter_inserter(
              std::back_inserter(ciw.session_matches()),
              SessionMatchesFilter()),
            SessionMatchesCleaner(channels, now)));
      }
      else if (add)
      {
        ChannelsInfoReader::ht_candidates_Container empty_ht;
        ciw.ht_candidates().reserve(ciw.ht_candidates().size() + add->ht_candidates().size());

        merge_htc_(
          ciw.ht_candidates(),
          channels,
          add->ht_candidates(),
          empty_ht);

        ciw.history_matches().reserve(ciw.history_matches().size() + add->history_matches().size());

        std::copy(
          add->history_matches().begin(), add->history_matches().end(),
          Algs::filter_inserter(
            std::back_inserter(ciw.history_matches()),
            ExistChannelFilter(channels)));

        ciw.history_visits().reserve(ciw.history_visits().size() + add->history_visits().size());

        std::copy(
          add->history_visits().begin(), add->history_visits().end(),
          Algs::filter_inserter(
            std::back_inserter(ciw.history_visits()),
            ExistChannelFilter(channels)));

        ciw.session_matches().reserve(ciw.session_matches().size() + add->session_matches().size());

        std::copy(
          add->session_matches().begin(), add->session_matches().end(),
          Algs::modify_inserter(
            Algs::filter_inserter(
              std::back_inserter(ciw.session_matches()),
              SessionMatchesFilter()),
            SessionMatchesCleaner(channels, now)));
      }
    }

    template <typename BaseReaderType, typename AddReaderType>
    void ChannelsMatcher::merge_htc_(
      ChannelsInfoWriter::ht_candidates_Container& htcw,
      const ChannelsHashMap& channels,
      const BaseReaderType& base,
      const AddReaderType& add)
    {
      typename BaseReaderType::const_iterator base_it = base.begin();
      typename AddReaderType::const_iterator add_it = add.begin();
      unsigned long tli_count = 0;

      unsigned long prev_channel_id = 0;

      while (base_it != base.end() && add_it != add.end())
      {
        if(prev_channel_id != (*base_it).channel_id())
        {
          prev_channel_id = (*base_it).channel_id();
          tli_count = 0;
        }
        
        if ((*base_it).channel_id() < (*add_it).channel_id())
        {
          if (channels.find((*base_it).channel_id()) != channels.end())
          {
            htcw.push_back(*base_it);
          }

          ++base_it;
        }
        else if ((*base_it).channel_id() > (*add_it).channel_id())
        {
          if (channels.find((*add_it).channel_id()) != channels.end())
          {
            htcw.push_back(*add_it);
          }

          ++add_it;
        }
        else
        {
          unsigned long channel_id = (*add_it).channel_id();
          ChannelsHashMap::const_iterator c_it = channels.find(channel_id);
          
          if (c_it != channels.end())
          {
            ChannelIntervalList::const_iterator ci_it = 
              c_it->second->today_long_intervals.begin();

            if (tli_count < c_it->second->today_long_intervals.size())
            {
              std::advance(ci_it, tli_count);

              int visits = static_cast<int>((*base_it).req_visits()) + 
                (*add_it).req_visits() - ci_it->min_visits;

              HTCandidatesWriter htc;
              htc.channel_id() = channel_id;
              htc.req_visits() = std::max(visits, 0);
              htc.visits() = (*base_it).visits() + (*add_it).visits();
              htc.weight() = htc.req_visits() == 0 ? ci_it->weight : 0;

              htcw.push_back(htc);
            }

            ++tli_count;
          }

          ++base_it;
          ++add_it;
        }
      }

      std::copy(
        base_it, base.end(),
        Algs::filter_inserter(
          std::back_inserter(htcw),
          ExistChannelFilter(channels)));

      std::copy(
        add_it, add.end(),
        Algs::filter_inserter(
          std::back_inserter(htcw),
          ExistChannelFilter(channels)));
    }

    void ChannelsMatcher::history_print(
      const Generics::MemBuf& mb,
      std::ostream& ostr,
      bool print_align)
      noexcept
    {
      ColumnProfilePrint::print_history_profile(
        mb.data(), mb.size(), ostr, print_align);
    }

    void ChannelsMatcher::print(
      const Generics::MemBuf& mb,
      std::ostream& ostr,
      bool print_expand,
      bool print_align)
      noexcept
    {
      ColumnProfilePrint::print_profile(
        mb.data(), mb.size(), ostr, print_expand, print_align);
    }
    
    void ChannelsMatcher::history_print(
      const void* profile,
      unsigned long size,
      std::ostream& ostr,
      bool print_align)
      noexcept
    {
      static const char* FUN = "ChannelsMatcher::history_print()";
      
      try
      {
        if (size == 0)
        {
          ostr << "History profile is empty." << std::endl << std::endl;
          return;
        }
        else
        {
          ColumnProfilePrint::print_history_profile(
            profile, size, ostr, print_align);
        }
      }
      catch (const eh::Exception& ex)
      {
        ostr << FUN << ": Attempt to print invalid profile. " << std::endl;
      }
    }

    void ChannelsMatcher::print(
      const void* profile,
      unsigned long size,
      std::ostream& ostr,
      bool print_expand,
      bool print_align)
      noexcept
    {
      static const char* FUN = "ChannelsMatcher::print()";
      
      try
      {
        if (size == 0)
        {
          ostr << "Profile is empty." << std::endl << std::endl;
          return;
        }
        else
        {
          ColumnProfilePrint::print_profile(
            profile, size, ostr, print_expand, print_align);
        }
      }
      catch (const eh::Exception& ex)
      {
        ostr << FUN << ": Attempt to print invalid profile. " << std::endl;
      }
    }

    void
    ChannelsMatcher::set_cohort_(
      std::string& res_cohort,
      const String::SubString& cohort_part1,
      const String::SubString& cohort_part2)
      noexcept
    {
      if(!cohort_part1.empty() || !cohort_part2.empty())
      {
        std::string res_cohort_part1;
        std::string res_cohort_part2;

        split_cohort_(
          res_cohort_part1,
          res_cohort_part2,
          res_cohort);

        if(!cohort_part1.empty())
        {
          res_cohort_part1 = cohort_part1.str();
        }

        if(!cohort_part2.empty())
        {
          res_cohort_part2 = cohort_part2.str();
        }

        res_cohort = res_cohort_part1 + "/" + res_cohort_part2;
      }
    }

    void
    ChannelsMatcher::split_cohort_(
      std::string& cohort_part1,
      std::string& cohort_part2,
      const String::SubString& cohort)
      noexcept
    {
      if(!cohort.empty())
      {
        auto pos = cohort.find('/');

        if(pos != String::SubString::NPOS)
        {
          cohort_part1 = cohort.substr(0, pos).str();
          cohort_part2 = cohort.substr(pos + 1).str();
        }
        else
        {
          cohort_part1 = cohort.str();
        }
      }
    }
  } /*namespace UserInfoSvcs*/
} /*namespace AdServer*/
