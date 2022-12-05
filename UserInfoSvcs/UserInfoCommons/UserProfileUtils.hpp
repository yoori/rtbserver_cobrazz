#ifndef USERINFOSVCS_USERINFOCOMMONS_USERPROFILEUTILS_HPP
#define USERINFOSVCS_USERINFOCOMMONS_USERPROFILEUTILS_HPP

#include <sstream>

#include <UtilCommons/Table.hpp>
#include <Commons/Algs.hpp>

#include "UserChannelBaseProfile.hpp"
#include "UserChannelHistoryProfile.hpp"

namespace AdServer
{
  namespace UserInfoSvcs
  {
    namespace
    {
      const Table::Column PROFILE_EXPANDED_TABLE_COLUMNS[] =
      {
        Table::Column("version", Table::Column::NUMBER),
        Table::Column("create_time", Table::Column::TEXT),
        Table::Column("history_time", Table::Column::TEXT),
        Table::Column("ignore_fraud_time", Table::Column::TEXT),
        Table::Column("last_request_time", Table::Column::TEXT),
        Table::Column("session_start_time", Table::Column::TEXT),
        Table::Column("household", Table::Column::NUMBER),
        Table::Column("first_colo_id", Table::Column::NUMBER),
        Table::Column("last_colo_id", Table::Column::NUMBER),
        Table::Column("cohort", Table::Column::TEXT),

        Table::Column("persistent_matches", Table::Column::TEXT),
        
        Table::Column("page_ht_candidates", Table::Column::TEXT),
        Table::Column("page_history_matches", Table::Column::TEXT),
        Table::Column("page_history_visits", Table::Column::TEXT),
        Table::Column("page_session_matches", Table::Column::TEXT),

        Table::Column("search_ht_candidates", Table::Column::TEXT),
        Table::Column("search_history_matches", Table::Column::TEXT),
        Table::Column("search_history_visits", Table::Column::TEXT),
        Table::Column("search_session_matches", Table::Column::TEXT),

        Table::Column("url_ht_candidates", Table::Column::TEXT),
        Table::Column("url_history_matches", Table::Column::TEXT),
        Table::Column("url_history_visits", Table::Column::TEXT),
        Table::Column("url_session_matches", Table::Column::TEXT),

        Table::Column("url_keyword_ht_candidates", Table::Column::TEXT),
        Table::Column("url_keyword_history_matches", Table::Column::TEXT),
        Table::Column("url_keyword_history_visits", Table::Column::TEXT),
        Table::Column("url_keyword_session_matches", Table::Column::TEXT),

        Table::Column("audience_channels", Table::Column::TEXT),
        
        Table::Column("last_page_triggers", Table::Column::TEXT),
        Table::Column("last_search_triggers", Table::Column::TEXT),
        Table::Column("last_url_triggers", Table::Column::TEXT),
        Table::Column("last_url_keyword_triggers", Table::Column::TEXT),

        Table::Column("geo_data", Table::Column::TEXT),
      };

      const Table::Column PROFILE_TABLE_COLUMNS[] =
      {
        Table::Column("version", Table::Column::NUMBER),
        Table::Column("create_time", Table::Column::TEXT),
        Table::Column("history_time", Table::Column::TEXT),
        Table::Column("ignore_fraud_time", Table::Column::TEXT),
        Table::Column("last_request_time", Table::Column::TEXT),
        Table::Column("session_start_time", Table::Column::TEXT),
        Table::Column("household", Table::Column::NUMBER),
        Table::Column("first_colo_id", Table::Column::NUMBER),
        Table::Column("last_colo_id", Table::Column::NUMBER),
        Table::Column("cohort", Table::Column::TEXT),
        
        Table::Column("persistent_matches", Table::Column::TEXT),
        
        Table::Column("page_ht_candidates", Table::Column::TEXT),
        Table::Column("page_history_matches", Table::Column::TEXT),
        Table::Column("page_history_visits", Table::Column::TEXT),
        Table::Column("page_session_matches", Table::Column::TEXT),

        Table::Column("search_ht_candidates", Table::Column::TEXT),
        Table::Column("search_history_matches", Table::Column::TEXT),
        Table::Column("search_history_visits", Table::Column::TEXT),
        Table::Column("search_session_matches", Table::Column::TEXT),
 
        Table::Column("url_ht_candidates", Table::Column::TEXT),
        Table::Column("url_history_matches", Table::Column::TEXT),
        Table::Column("url_history_visits", Table::Column::TEXT),
        Table::Column("url_session_matches", Table::Column::TEXT),

        Table::Column("url_keyword_ht_candidates", Table::Column::TEXT),
        Table::Column("url_keyword_history_matches", Table::Column::TEXT),
        Table::Column("url_keyword_history_visits", Table::Column::TEXT),
        Table::Column("url_keyword_session_matches", Table::Column::TEXT),

        Table::Column("audience_channels", Table::Column::TEXT),

        Table::Column("last_page_triggers", Table::Column::TEXT),
        Table::Column("last_search_triggers", Table::Column::TEXT),
        Table::Column("last_url_triggers", Table::Column::TEXT),
        Table::Column("last_url_keyword_triggers", Table::Column::TEXT),

        Table::Column("geo_data", Table::Column::TEXT),
      };

      const Table::Column HISTORY_PROFILE_TABLE_COLUMNS[] =
      {
        Table::Column("major_version", Table::Column::NUMBER),
        Table::Column("minor_version", Table::Column::NUMBER),

        Table::Column("page_channels", Table::Column::TEXT),
        Table::Column("search_channels", Table::Column::TEXT),
        Table::Column("url_channels", Table::Column::TEXT),
        Table::Column("url_keyword_channels", Table::Column::TEXT),
      };
    }

    class ColumnProfilePrint
    {
    public:

      static void print_profile(
        const void* profile,
        unsigned long size,
        std::ostream& ostr,
        bool expand = false,
        bool align = false)
        noexcept
      {
        if (size != 0)
        {
          unsigned long columns = expand ?
            sizeof(PROFILE_EXPANDED_TABLE_COLUMNS) /
            sizeof(PROFILE_EXPANDED_TABLE_COLUMNS[0]) :
            sizeof(PROFILE_TABLE_COLUMNS) / sizeof(PROFILE_TABLE_COLUMNS[0]);
          
          Table table(columns);
          
          for(unsigned long i = 0; i < columns; i++)
          {
            table.column(i, expand ?
                         PROFILE_EXPANDED_TABLE_COLUMNS[i] : PROFILE_TABLE_COLUMNS[i]);
          }
          
          Table::Row row(table.columns());
          
          ChannelsProfileReader reader(profile, size);

          std::string space_str(table.value_align() + 3, ' ');
            
          std::ostringstream space_align;
          
          if (align)
          {
            space_align << std::endl << space_str.c_str();
          }
          else
          {
            space_align << " ";
          }
          
          {
            row.add_field(reader.version());
            row.add_field(
              Generics::Time(reader.create_time()).gm_ft());
            row.add_field(
              Generics::Time(reader.history_time()).gm_ft());
            row.add_field(
              Generics::Time(
                reader.ignore_fraud_time()).gm_ft());
            row.add_field(
              Generics::Time(
                reader.last_request_time()).gm_ft());
            row.add_field(
              Generics::Time(
                reader.session_start()).gm_ft());
            row.add_field(reader.household());
            row.add_field(reader.first_colo_id());
            row.add_field(reader.last_colo_id());
            row.add_field(reader.cohort());
            
            std::ostringstream pm_ostr;
            for(PersistentMatchesReader::channel_ids_Container::const_iterator it =
                  reader.persistent_matches().channel_ids().begin();
                it != reader.persistent_matches().channel_ids().end(); ++it)
            {
              pm_ostr <<
                (it == reader.persistent_matches().channel_ids().begin() ?
                 "" : space_align.str().c_str()) << *it;
            }
            
            row.add_field(pm_ostr.str());
            add_channel_info_row(
              row, reader.page_channels(), space_align.str().c_str(), expand);
            add_channel_info_row(
              row, reader.search_channels(), space_align.str().c_str(), expand);
            add_channel_info_row(
              row, reader.url_channels(), space_align.str().c_str(), expand);
            add_channel_info_row(
              row, reader.url_keyword_channels(), space_align.str().c_str(), expand);

            add_audience_data_row(
              row,
              reader.audience_channels().begin(),
              reader.audience_channels().end(),
              space_align.str().c_str());
            
            add_last_triggers_row(
              row,
              reader.last_page_triggers().begin(),
              reader.last_page_triggers().end(),
              space_align.str().c_str());

            add_last_triggers_row(
              row,
              reader.last_search_triggers().begin(),
              reader.last_search_triggers().end(),
              space_align.str().c_str());

            add_last_triggers_row(
              row,
              reader.last_url_triggers().begin(),
              reader.last_url_triggers().end(),
              space_align.str().c_str());

            add_last_triggers_row(
              row,
              reader.last_url_keyword_triggers().begin(),
              reader.last_url_keyword_triggers().end(),
              space_align.str().c_str());

            add_geo_data_row(
              row,
              reader.geo_data().begin(),
              reader.geo_data().end(),
              space_align.str().c_str());
          }

          table.add_row(row);
          table.dump(ostr);
        }
      }
      
      static void print_history_profile(
        const void* profile,
        unsigned long size,
        std::ostream& ostr,
        bool align = false)
        noexcept
      {
        if (size != 0)
        {
          unsigned long columns =
            sizeof(HISTORY_PROFILE_TABLE_COLUMNS) /
            sizeof(HISTORY_PROFILE_TABLE_COLUMNS[0]);
          
          Table table(columns);
          
          for(unsigned long i = 0; i < columns; i++)
          {
            table.column(i, HISTORY_PROFILE_TABLE_COLUMNS[i]);
          }
          
          Table::Row row(table.columns());
          
          HistoryUserProfileReader reader(profile, size);
          
          {
            row.add_field(reader.major_version());
            row.add_field(reader.minor_version());

            std::string space_str(table.value_align() + 3, ' ');
            
            std::ostringstream space_align;
            
            if (align)
            {
              space_align << std::endl << space_str.c_str();
            }
            else
            {
              space_align << " ";
            }
            
            add_history_info_row(
              row, reader.page_channels(), space_align.str().c_str());
            add_history_info_row(
              row, reader.search_channels(), space_align.str().c_str());
            add_history_info_row(
              row, reader.url_channels(), space_align.str().c_str());
            add_history_info_row(
              row, reader.url_keyword_channels(), space_align.str().c_str());
          }
          
          table.add_row(row);
          table.dump(ostr);
        }
      }
      
    private:

      static void add_history_info_row(
        Table::Row& row,
        const HistoryUserProfileReader::page_channels_Container& rdr,
        const char* space_align)
      {
        std::ostringstream ostr;
        
        for(HistoryUserProfileReader::page_channels_Container::const_iterator it =
              rdr.begin(); it != rdr.end(); ++it)
        {
          ostr << (it == rdr.begin() ? "" : space_align) <<
            "[ channel_id = " << (*it).channel_id() <<
            ", days_visits_pairs =";
          
          for(HistoryChannelInfoReader::days_visits_Container::const_iterator dv_it =
                (*it).days_visits().begin();
              dv_it != (*it).days_visits().end(); ++dv_it)
          {
            ostr << (dv_it == (*it).days_visits().begin() ? " " : ", ") <<
              (*dv_it).days() << ":" << (*dv_it).visits();
          }
          
          ostr << " ]";
        }
        
        row.add_field(ostr.str());
      }

      template<typename IteratorType>
      static void add_last_triggers_row(
        Table::Row& row,
        IteratorType it_begin,
        IteratorType it_end,
        const char* space_align)
      {
        std::ostringstream ostr;
        IteratorType it = it_begin;
        
        while (it != it_end)
        {
          ostr << (it == it_begin ? "" : space_align)
               << "[ channel_id = " << (*it).channel_id()
               << ", channel_trigger_id = " << (*it).channel_trigger_id()
               << ", last_match_time = " << Generics::Time((*it).last_match_time()).gm_ft()
               << " ]";
          
          ++it;
        }

        row.add_field(ostr.str());
      }

      template<typename IteratorType>
      static void add_geo_data_row(
        Table::Row& row,
        IteratorType it_begin,
        IteratorType it_end,
        const char* space_align)
      {
        std::ostringstream ostr;
        IteratorType it = it_begin;
        
        while (it != it_end)
        {
          AdServer::CampaignSvcs::CoordDecimal latitude;
          latitude.unpack((*it).latitude().get());
          AdServer::CampaignSvcs::CoordDecimal longitude;
          longitude.unpack((*it).longitude().get());
          AdServer::CampaignSvcs::AccuracyDecimal accuracy;
          accuracy.unpack((*it).accuracy().get());
          
          ostr << (it == it_begin ? "" : space_align)
               << "[ latitude = " << latitude
               << ", longitude = " << longitude
               << ", accuracy = " << accuracy
               << ", timestamp = " << Generics::Time((*it).timestamp()).gm_ft()
               << " ]";
          
          ++it;
        }

        row.add_field(ostr.str());
      }

      template<typename IteratorType>
      static void add_audience_data_row(
        Table::Row& row,
        IteratorType it_begin,
        IteratorType it_end,
        const char* space_align)
      {
        std::ostringstream ostr;
        IteratorType it = it_begin;
        
        while (it != it_end)
        {
          ostr << (it == it_begin ? "" : space_align)
               << "[ channel_id = " << (*it).channel_id()
               << ", time = " << Generics::Time((*it).time()).gm_ft()
               << " ]";
          
          ++it;
        }

        row.add_field(ostr.str());
      } 
      
      static void add_channel_info_row(
        Table::Row& row,
        const ChannelsInfoReader& rdr,
        const char* space_align,
        bool /*expand*/)
      {
        std::ostringstream htc_ostr;
        for(ChannelsInfoReader::ht_candidates_Container::const_iterator it =
              rdr.ht_candidates().begin();
            it != rdr.ht_candidates().end(); ++it)
        {
          htc_ostr << (it == rdr.ht_candidates().begin() ? "" : space_align) <<
            "[ channel_id = " << (*it).channel_id() <<
            ", req_visits = " << (*it).req_visits() <<
            ", visits = " << (*it).visits() <<
            ", weight = " << (*it).weight() << " ]";
        }
        
        row.add_field(htc_ostr.str());
        
        std::ostringstream hm_ostr;
        for(ChannelsInfoReader::history_matches_Container::const_iterator it =
              rdr.history_matches().begin();
            it != rdr.history_matches().end(); ++it)
        {
          hm_ostr << (it == rdr.history_matches().begin() ? "" : space_align) <<
            "[ channel_id = " << (*it).channel_id() <<
            ", weight = " << (*it).weight() << " ]";
        }
        
        row.add_field(hm_ostr.str());
        
        std::ostringstream hv_ostr;
        for(ChannelsInfoReader::history_visits_Container::const_iterator it =
              rdr.history_visits().begin();
            it != rdr.history_visits().end(); ++it)
        {
          hv_ostr << (it == rdr.history_visits().begin() ? "" : space_align) <<
            "[ channel_id = " << (*it).channel_id() <<
            ", visits = " << (*it).visits() << " ]";
        }
        
        row.add_field(hv_ostr.str());
        
        std::ostringstream sm_ostr;
        for(ChannelsInfoReader::session_matches_Container::const_iterator it =
              rdr.session_matches().begin();
            it != rdr.session_matches().end(); ++it)
        {
          sm_ostr << (it == rdr.session_matches().begin() ? "" : space_align) <<
            "[ channel_id = " << (*it).channel_id() << ", timestamps =";
          
          for(SessionMatchesReader::timestamps_Container::const_iterator ts_it =
                (*it).timestamps().begin();
              ts_it != (*it).timestamps().end(); ++ts_it)
          {
            sm_ostr << (ts_it == (*it).timestamps().begin() ? " " : ", ") <<
              Generics::Time(*ts_it).gm_ft();
          }
          
          sm_ostr << " ]";
        }
        
        row.add_field(sm_ostr.str());
      }
    };
    
  }   // namespace UserInfoSvcs
}   // namespace AdServer



#endif
