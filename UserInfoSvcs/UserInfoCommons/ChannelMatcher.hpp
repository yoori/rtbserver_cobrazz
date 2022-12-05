#ifndef _USER_INFO_SVCS_CHANNEL_MATCHER_HPP_
#define _USER_INFO_SVCS_CHANNEL_MATCHER_HPP_

#include <iostream>
#include <vector>
#include <list>
#include <set>

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/MemBuf.hpp>

#include <Commons/Algs.hpp>

#include <UserInfoSvcs/UserInfoCommons/UserChannelBaseProfile.hpp>
#include "Allocator.hpp"
#include "ChannelDictionary.hpp"
#include <UserInfoSvcs/UserInfoCommons/UserChannelHistoryProfile.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    const unsigned long UNKNOWN_COLO_ID = 0;
    const uint32_t DEFAULT_COLO = -1;

    const unsigned long MAX_GEO_DATA = 1;

    typedef std::vector<unsigned long> ChannelIdVector;

    typedef std::set<
      unsigned long,
      std::less<unsigned long>,
      Generics::TAlloc::ThreadPool<unsigned long, 256> >
    ChannelIdSet;

    struct AudienceChannel
    {
      bool operator<(const AudienceChannel& right) const
      {
        return channel_id < right.channel_id;
      }

      unsigned long channel_id;
      Generics::Time time;
    };
    typedef std::set<AudienceChannel> AudienceChannelSet;

    struct UniqueChannels
    {
      ChannelIdSet unique_channels;
      ChannelIdSet session_channels;
      ChannelIdSet history_channels;
    };

    struct AllUniqueChannels
    {
      UniqueChannels simple;
      UniqueChannels discover;
      
      ChannelIdSet found_session_channels;
      ChannelIdSet found_history_channels;
    };

    enum UniqueType
    {
      SESSION = 0,
      HISTORY
    };

    const unsigned long CURRENT_BASE_PROFILE_VERSION = 340;
    const unsigned long CURRENT_HISTORY_MAJOR_PROFILE_VERSION = 330;
    const unsigned long CURRENT_HISTORY_MINOR_PROFILE_VERSION = 0;

    const unsigned long SEC_IN_DAY = 86400;

    struct ChannelIdPack
    {
      ChannelIdVector page_channels;
      ChannelIdVector search_channels;
      ChannelIdVector url_channels;
      ChannelIdVector url_keyword_channels;
      
      ChannelIdVector persistent_channels;
    };

    struct ChannelMatch
    {
      ChannelMatch(unsigned long test_id)            // for unit tests only!
        :
        channel_id(test_id),
        channel_trigger_id(test_id)
      {}
      
      ChannelMatch(unsigned long channel_id_val,
                   unsigned long channel_trigger_id_val)
        :
        channel_id(channel_id_val),
        channel_trigger_id(channel_trigger_id_val)
      {}

      bool operator<(const ChannelMatch& right) const
      {
        return
          (channel_id < right.channel_id ||
           (channel_id == right.channel_id &&
            channel_trigger_id < right.channel_trigger_id));
      }

      unsigned long channel_id;
      unsigned long channel_trigger_id;
    };
    typedef std::vector<ChannelMatch> ChannelMatchVector;

    struct ChannelMatchPack
    {
      ChannelMatchVector page_channels;
      ChannelMatchVector search_channels;
      ChannelMatchVector url_channels;
      ChannelMatchVector url_keyword_channels;
      ChannelMatchVector audience_channels;
      
      ChannelIdVector persistent_channels;
    };

    struct UniqueChannelsResult
    {
      UniqueChannelsResult()
        : simple_channels(0),
          session_simple_channels(0),
          history_simple_channels(0),
          discover_channels(0),
          session_discover_channels(0),
          history_discover_channels(0)
      {}

      void init(const AllUniqueChannels& auc)
      {
        simple_channels = auc.simple.unique_channels.size();
        session_simple_channels = auc.simple.session_channels.size();
        history_simple_channels = auc.simple.history_channels.size();
        discover_channels = auc.discover.unique_channels.size();
        session_discover_channels = auc.discover.session_channels.size();
        history_discover_channels = auc.discover.history_channels.size();
      }

      bool operator!=(const UniqueChannelsResult& right) const noexcept
      {
        return
          simple_channels != right.simple_channels ||
          session_simple_channels != right.session_simple_channels ||
          history_simple_channels != right.history_simple_channels ||
          discover_channels != right.discover_channels ||
          session_discover_channels != right.session_discover_channels ||
          history_discover_channels != right.history_discover_channels;
      }
      
      unsigned long simple_channels;
      unsigned long session_simple_channels;
      unsigned long history_simple_channels;
      unsigned long discover_channels;
      unsigned long session_discover_channels;
      unsigned long history_discover_channels;
    };

    struct GeoDataResult
    {
      AdServer::CampaignSvcs::CoordDecimal latitude;
      AdServer::CampaignSvcs::CoordDecimal longitude;
      AdServer::CampaignSvcs::AccuracyDecimal accuracy;
    };
    typedef std::list<GeoDataResult> GeoDataResultList;
    
    struct ProfileProperties
    {
      bool fraud_request;
      std::string cohort;
      std::string cohort2;

      GeoDataResultList geo_data_list;
    };

    struct CoordData
    {
      CoordData()
        :
        defined(false)
      {}
      
      bool defined;
      
      AdServer::CampaignSvcs::CoordDecimal latitude;
      AdServer::CampaignSvcs::CoordDecimal longitude;
      AdServer::CampaignSvcs::AccuracyDecimal accuracy;
    };
    
    struct ProfileMatchParams
    {
      ProfileMatchParams(
        const String::SubString& cohort_val = String::SubString(),
        const String::SubString& cohort2_val = String::SubString(),
        const Generics::Time& repeat_trigger_timeout_val = Generics::Time::ZERO,
        bool filter_contextual_triggers_val = false,
        bool no_match_val = false,
        bool no_result_val = false,
        bool provide_channel_count_val = false,
        bool provide_persistent_channels_val = false,
        bool change_last_request_val = true,
        bool household_val = false,
        long request_colo_id_val = UNKNOWN_COLO_ID,
        const CoordData* coord_data_val = 0)
        : no_match(no_match_val),
          no_result(no_result_val),
          provide_channel_count(provide_channel_count_val),
          provide_persistent_channels(provide_persistent_channels_val),
          change_last_request(change_last_request_val),
          household(household_val),
          request_colo_id(request_colo_id_val),
          cohort(cohort_val.str()),
          cohort2(cohort2_val.str()),
          repeat_trigger_timeout(repeat_trigger_timeout_val),
          filter_contextual_triggers(filter_contextual_triggers_val)
      {
        if (coord_data_val != 0)
        {
          coord_data.defined = true;

          coord_data.latitude = coord_data_val->latitude;
          coord_data.longitude = coord_data_val->longitude;
          coord_data.accuracy = coord_data_val->accuracy;
        }
        else
        {
          coord_data.defined = false;
        }
      }
      
      bool no_match;
      bool no_result;
      bool provide_channel_count;
      bool provide_persistent_channels;
      bool change_last_request;
      bool household;
      long request_colo_id;
      std::string cohort;
      std::string cohort2;
      Generics::Time repeat_trigger_timeout;
      bool filter_contextual_triggers;

      CoordData coord_data;
    };

    inline
    bool operator<(
      const LastTriggerReader& last_trigger_rdr,
      const ChannelMatch& channel)
    {
      return
        (last_trigger_rdr.channel_id() < channel.channel_id ||
         (last_trigger_rdr.channel_id() == channel.channel_id &&
          last_trigger_rdr.channel_trigger_id() < channel.channel_trigger_id));
    }
    
    inline
    bool operator==(
      const LastTriggerReader& last_trigger_rdr,
      const ChannelMatch& channel)
    {
      return
        (last_trigger_rdr.channel_id() == channel.channel_id &&
         last_trigger_rdr.channel_trigger_id() == channel.channel_trigger_id);
    }
    
    class ChannelsMatcher
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(InvalidProfileException, Exception);

      ChannelsMatcher();
      
      ChannelsMatcher(
        Generics::SmartMemBuf* base_profile,
        Generics::SmartMemBuf* add_profile) noexcept;

      static void unique_channels(
        const Generics::MemBuf& base_profile,
        const Generics::MemBuf* history_profile,
        const ChannelDictionary& dictionary,
        UniqueChannelsResult& ucr)
        /*throw(InvalidProfileException)*/;

      Generics::Time last_request() const
        /*throw(InvalidProfileException)*/;

      Generics::Time create_time() const
        /*throw(InvalidProfileException)*/;

      Generics::Time session_start() const
        /*throw(InvalidProfileException)*/;

      bool fraud_user(const Generics::Time& now);

      void add_audience_channels(
        const AudienceChannelSet& audience_channels,
        const ChannelDictionary& channels,
        const Generics::Time& expire_time)
        /*throw(InvalidProfileException)*/;

      void remove_audience_channels(
        const AudienceChannelSet& audience_channels)
        /*throw(InvalidProfileException)*/;
      
      void match(
        ChannelMatchMap& result_channels,
        const Generics::Time& now,
        const ChannelMatchPack& channels_pack,
        const ChannelDictionary& channels,
        const ProfileMatchParams& profile_match_params,
        ProfileProperties& properties,
        const Generics::Time& session_timeout,
        bool match_to_add = false) /*throw(InvalidProfileException)*/;

      template <typename WriterType, typename ReaderType>
      static void copy_history_section(
        WriterType& ciw, const ReaderType& cir) noexcept
      {
        std::copy(
          cir.begin(), cir.end(),
          Algs::modify_inserter(
            std::back_inserter(ciw),
            Algs::MemoryInitAdapter<HistoryChannelInfoWriter>()));
      }
      
      template <typename ReaderType>
      static void copy_base_section(
        ChannelsInfoWriter& ciw,
        const ReaderType& cir) noexcept
      {
        std::copy(
          cir.ht_candidates().begin(),
          cir.ht_candidates().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.ht_candidates()),
            Algs::MemoryInitAdapter<HTCandidatesWriter>()));
        
        std::copy(
          cir.history_matches().begin(),
          cir.history_matches().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.history_matches()),
            Algs::MemoryInitAdapter<HistoryMatchesWriter>()));
        
        std::copy(
          cir.history_visits().begin(),
          cir.history_visits().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.history_visits()),
            Algs::MemoryInitAdapter<HistoryVisitsWriter>()));
        
        std::copy(
          cir.session_matches().begin(),
          cir.session_matches().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.session_matches()),
            Algs::MemoryInitAdapter<SessionMatchesWriter>()));
      }

      template <typename ReaderType>
      static void copy_section(
        ChannelsInfoWriter& ciw,
        const ReaderType& cir) noexcept
      {
        copy_base_section(ciw, cir);
      }
      
      void merge(
        Generics::SmartMemBuf* history_profile,
        const Generics::MemBuf& other_base_profile,
        const Generics::MemBuf& other_history_profile,
        const ChannelDictionary& channels,
        const ProfileMatchParams& profile_match_params,
        const Generics::Time& merge_time = Generics::Time::ZERO)
        /*throw(InvalidProfileException)*/;

      /** history optimize profile,
       *  and fill partly match result for history channels */
      void history_optimize(
        Generics::SmartMemBuf* history_profile,
        const Generics::Time& now,
        const Generics::Time& gmt_offset,
        const ChannelDictionary& channels,
        bool* first_today_history_optimization = 0);

      bool need_channel_count_stats_logging(
        const Generics::Time& now,
        const Generics::Time& gmt_offset) const
        /*throw(InvalidProfileException)*/;
      
      bool need_history_optimization(
        const Generics::Time& now,
        const Generics::Time& period,
        const Generics::Time& gmt_offset)
        const
        /*throw(InvalidProfileException)*/;

      static void history_print(
        const Generics::MemBuf& mb,
        std::ostream& ostr,
        bool print_align = false)
        noexcept;

      static void print(
        const void* profile,
        unsigned long size,
        std::ostream& ostr,
        bool print_expand = false,
        bool print_align = false)
        noexcept;

      static void history_print(
        const void* profile,
        unsigned long size,
        std::ostream& ostr,
        bool print_align = false)
        noexcept;
      
      static void print(
        const Generics::MemBuf& mb,
        std::ostream& ostr,
        bool print_expand = false,
        bool print_align = false)
        noexcept;

      static void delete_excess_timestamps_(
        SessionMatchesWriter::timestamps_Container& wr,
        const ChannelIntervalList& cil) noexcept;

    private:
      void
      write_geo_data_(
        GeoDataWriter& gdw,
        const CoordData& coord_data,
        const Generics::Time& now);

      AdServer::CampaignSvcs::AccuracyDecimal
      read_accuracy_(
        const void* buf);

      AdServer::CampaignSvcs::CoordDecimal
      read_coord_(
        const void* buf);
      
      template <typename _T1, typename _T2>
      void
      merge_geo_data_(
        ChannelsProfileWriter::geo_data_Container& res_geo_data,
        const _T1& base_geo_data,
        const _T2& add_geo_data,
        const Generics::Time& now);
      
      template <typename _T1, typename _T2>
      void
      add_geo_data_(
        ChannelsProfileWriter::geo_data_Container& res_geo_data,
        const _T1& geo_data_array,
        const _T2& new_geo_data);

      bool
      merge_triggers_section_(
        ChannelIdVector& result_filtered_channels,
        ChannelsProfileWriter::last_page_triggers_Container& last_triggers,
        const ChannelMatchVector& triggered_channels,
        const ChannelsProfileReader::last_page_triggers_Container& last_triggers_rdr,
        const ChannelsHashMap& channels,
        const Generics::Time& now,
        const Generics::Time& repeat_trigger_timeout,
        bool filter_contextual_triggers);
      
      bool
      update_triggers_(
        ChannelIdPack& result_filtered_channels,
        ChannelsProfileWriter& result_profile_triggers,
        const ChannelMatchPack& triggered_channels,
        const ChannelsProfileReader* profile_reader,
        const ChannelDictionary& channels,
        const Generics::Time& now,
        const Generics::Time& repeat_trigger_timeout,
        bool filter_contextual_triggers)
        /*throw(InvalidProfileException)*/;
      
      static void collect_channel_ids_(
        const Generics::MemBuf& base_profile,
        const ChannelDictionary& dictionary,
        AllUniqueChannels& auc)
        /*throw(Exception)*/;

      static void collect_history_channel_ids_(
        const Generics::MemBuf& history_profile,
        const ChannelDictionary& dictionary,
        AllUniqueChannels& auc)
        /*throw(Exception)*/;
      
      static void fill_unique_channels_(
        const ChannelsInfoReader& section,
        const ChannelDictionary& dictionary,
        AllUniqueChannels& auc)
        /*throw(Exception)*/;

      template<typename ContainerType>
      static void process_channels_sequence_(
        const ContainerType& sequence,
        const ChannelDictionary& dictionary,
        UniqueType channels_type,
        AllUniqueChannels& auc)
        /*throw(Exception)*/;
      
      bool need_history_optimization_(
        const Generics::SmartMemBuf* profile,
        const Generics::Time& now,
        const Generics::Time& period,
        const Generics::Time& gmt_offset)
        const
        /*throw(InvalidProfileException)*/;

      template <typename BaseChannelsInfoType, typename AddChannelsInfoType>
      void merge_persistent_channels_(
        PersistentMatchesWriter& pmw,
        const BaseChannelsInfoType* base,
        const AddChannelsInfoType* add);
      
      void match_persistent_section_(
        ChannelMatchMap& result_channels,
        PersistentMatchesWriter* out_pmw,
        PersistentMatchesWriter* match_pmw,
        const PersistentMatchesReader* base_in,
        const PersistentMatchesReader* add_in,
        const ChannelIdVector& channels,
        bool match_to_add,
        bool provide_persistent_channels) /*throw(Exception)*/;
      
      void match_section_(
        ChannelMatchMap& result_channels,
        ChannelsInfoWriter* out_ciw,
        ChannelsInfoWriter* match_ciw,
        const ChannelsInfoReader* base_in,
        const ChannelsInfoReader* add_in,
        const ChannelIdVector& channels,
        const ChannelsHashMap& dictionary,
        const Generics::Time& now,
        bool match_to_add,
        bool household = false) /*throw(Exception)*/;

      void fill_channels_results_(
        ChannelMatchMap& result_channels,
        const ChannelsInfoWriter& ciw,
        const Generics::Time& now,
        const ChannelsHashMap& channels);

      void add_weight_(
        ChannelMatchMap& result_channels,
        uint32_t index,
        uint32_t weight);

      unsigned long history_matched_(
        const HistoryChannelInfoWriter& hciw,
        const ChannelsHashMap& channels);

      void fill_history_candidate_visits_(
        ChannelsInfoWriter::ht_candidates_Container& ht_candidates,
        const HistoryChannelInfoWriter& hciw,
        const ChannelsHashMap& channels);

      template <typename HistoryChannelInfoList>
      void fill_history_candidates_(
        ChannelsInfoWriter& ciw,
        const HistoryChannelInfoList& hciw,
        const ChannelsHashMap& channels);

      void add_channel_visit_(
        const Generics::Time& now,
        const unsigned long channel_id,
        const ChannelsHashMap& channels,
        ChannelsInfoWriter& ciw,
        ChannelMatchMap& result_channels) noexcept;
    
      void merge_history_info_(
        ChannelsInfoWriter::history_visits_Container& wr,
        const ChannelsInfoReader::history_visits_Container& rdr,
        uint32_t days,
        const ChannelsHashMap& channels);

      template <typename HistoryChannelInfoListWriter,
                typename HistoryChannelInfoListReader>
      void merge_history_data_(
        HistoryChannelInfoListWriter& hw,
        const HistoryChannelInfoListReader& base,
        const HistoryChannelInfoListReader& add,
        const Generics::Time& now,
        const ChannelsHashMap& channels);

      template <typename BaseProfileType, typename AddProfileType>
      void merge_(
        ChannelsProfileWriter& upw,
        const ChannelDictionary& channels,
        const Generics::Time& now,
        const BaseProfileType* base,
        const AddProfileType* add)
        /*throw(InvalidProfileException)*/;

      template <typename BaseChannelsInfoType, typename AddChannelsInfoType>
      void merge_channels_info_(
        ChannelsInfoWriter& ciw,
        const ChannelsHashMap& channels,
        const Generics::Time& now,
        const BaseChannelsInfoType* base,
        const AddChannelsInfoType* add,
        bool household = false);
 
      template <typename BaseReaderType, typename AddReaderType>
      void merge_htc_(
        ChannelsInfoWriter::ht_candidates_Container& htcw,
        const ChannelsHashMap& channels,
        const BaseReaderType& base,
        const AddReaderType& add);

      template <typename HistoryChannelsInfoListWriter,
                typename HistoryChannelsInfoListReader>
      void history_optimize_(
        ChannelsInfoWriter& channels_info_writer,
        HistoryChannelsInfoListWriter& history_channels_writer,
        const HistoryChannelsInfoListReader& history_channels_reader,
        const ChannelsInfoReader& channels_info_reader,
        const ChannelsHashMap& channels,
        unsigned long days,
        const Generics::Time& now,
        bool household = false);

      static void
      set_cohort_(
        std::string& res_cohort,
        const String::SubString& cohort_part1,
        const String::SubString& cohort_part2)
        noexcept;

      static void
      split_cohort_(
        std::string& cohort_part1,
        std::string& cohort_part2,
        const String::SubString& cohort)
        noexcept;

    private:
      Generics::SmartMemBuf_var base_profile_;
      Generics::SmartMemBuf_var add_profile_;

      bool no_result_;
    };
  }
}

#include "ChannelMatcher.ipp"

#endif
