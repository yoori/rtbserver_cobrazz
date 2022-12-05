#include "UserInventoryContainer.hpp"

#include <PrivacyFilter/Filter.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>

#include "Compatibility/UserChannelInventoryProfileAdapter.hpp"

#include "Algs.hpp"

namespace Aspect
{
  const char USER_INVENTORY_CONTAINER[] = "UserInventoryContainer";
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  /*
   * get_inv_day(...) : get profile day specific section
   * fill_channel_ecpms(...) : fill InventoryInfo 'channel by ecpm' section by day
   * fill_channel_appears(...) : fill InventoryInfo 'channel appears' section by day
   *   fill_channel_typed_appears(...)
   * clear_expired_days(...) : clear expired days information
   */
  namespace
  {
    const RevenueDecimal CPM_FACTOR = RevenueDecimal(
      1000 * ChannelECPM::CPM_PRECISION, 0);
  }

  struct DateCompare
  {
    bool operator()(
      const ChannelInventoryDayWriter& left,
      const Generics::Time& right) const noexcept
    {
      return Generics::Time(left.date()) < right;
    }

    bool operator()(
      const Generics::Time& left,
      const ChannelInventoryDayWriter& right) const noexcept
    {
      return left < Generics::Time(right.date());
    }
  };

  void
  init_profile_writer(
    UserChannelInventoryProfileWriter& profile_writer)
    noexcept
  {
    profile_writer.version() = CURRENT_CHANNEL_INVENTORY_PROFILE_VERSION;
    profile_writer.create_time() = 0;
    profile_writer.last_request_time() = 0;
    profile_writer.last_daily_processing_time() = 0;
  }

  ChannelInventoryDayWriter&
  get_inv_day(
    bool& profile_changed,
    UserChannelInventoryProfileWriter& profile_writer,
    const Generics::Time& date)
    /*throw(PlainTypes::CorruptedStruct)*/
  {
    typedef UserChannelInventoryProfileWriter::days_Container DayList;

    DayList::iterator day_it =
      std::lower_bound(
        profile_writer.days().begin(),
        profile_writer.days().end(),
        date,
        DateCompare());

    if(day_it == profile_writer.days().end() ||
      date != Generics::Time(day_it->date()))
    {
      profile_changed = true;

      ChannelInventoryDayWriter new_writer;
      new_writer.date() = date.tv_sec;
      return *profile_writer.days().insert(day_it, new_writer);
    }

    return *day_it;
  }

  void
  fill_channel_ecpms(
    bool& profile_changed,
    InventoryActionProcessor::InventoryInfo& inv_info,
    ChannelInventoryDayWriter& profile_writer,
    const MatchRequestProcessor::MatchInfo& inv_request_info)
    /*throw(eh::Exception, PlainTypes::CorruptedStruct)*/
  {
    // Skip ChannelInventoryByCPM filling (ADSC-10002)
    if (inv_request_info.auction_type == CampaignSvcs::AT_RANDOM)
    {
      return;
    }
    // TODO: remove SizeChannelSet creation (this create CPU load)
    //   for do this profile_writer.channel_price_ranges must be ordered
    //   and inventory profile compatibility stub required
    //
    typedef std::map<Commons::ImmutableString, ChannelIdSet> SizeChannelSet;

    inv_info.impop_channels.insert(
      inv_info.impop_channels.end(),
      inv_request_info.triggered_cpm_expression_channels.begin(),
      inv_request_info.triggered_cpm_expression_channels.end());

    unsigned long check_ecpm = 0;

    if(inv_request_info.display_ad.present())
    {
      check_ecpm = RevenueDecimal::mul(
        inv_request_info.display_ad->avg_revenue,
        CPM_FACTOR,
        Generics::DMR_FLOOR).ceil(0).integer<unsigned long>();
    }
    else if(!inv_request_info.text_ads.empty())
    {
      RevenueDecimal sum_max_avg_revenue = RevenueDecimal::ZERO;
      for(MatchRequestProcessor::MatchInfo::AdBidSlotList::
            const_iterator text_ad_it = inv_request_info.text_ads.begin();
          text_ad_it != inv_request_info.text_ads.end(); ++text_ad_it)
      {
        sum_max_avg_revenue += RevenueDecimal::mul(
          text_ad_it->max_avg_revenue,
          CPM_FACTOR,
          Generics::DMR_FLOOR);
      }

      check_ecpm = sum_max_avg_revenue.ceil(0).integer<unsigned long>();
    }
    else
    {
      check_ecpm = RevenueDecimal::mul(
        inv_request_info.cost_threshold,
        CPM_FACTOR,
        Generics::DMR_FLOOR).ceil(0).integer<unsigned long>();
    }

    inv_info.impop_ecpm = check_ecpm;

    SizeChannelSet size_channels;

    for(StringSet::const_iterator size_it = inv_request_info.sizes.begin();
        size_it != inv_request_info.sizes.end(); ++size_it)
    {
      size_channels[*size_it] = inv_request_info.triggered_cpm_expression_channels;
    }

    // fill disappear channels and erase channels
    for(std::list<ChInvByCMPCellWriter>::iterator
          it = profile_writer.channel_price_ranges().begin();
        it != profile_writer.channel_price_ranges().end(); )
    {
      if (inv_request_info.country_code.str() == (*it).country())
      {
        SizeChannelSet::iterator size_it =
          size_channels.find(Commons::ImmutableString((*it).tag_size()));

        if(size_it != size_channels.end())
        {
          // cell found
          ChInvByCMPCellWriter::channel_list_Container& channels =
            (*it).channel_list();

          // remove_if semantics
          auto result_it = channels.begin();

          for(ChInvByCMPCellWriter::channel_list_Container::const_iterator
                ch_it = channels.begin();
              ch_it != channels.end(); ++ch_it)
          {
            ChannelIdSet::iterator fit = size_it->second.find(*ch_it);

            if(fit != size_it->second.end())
            {
              if(check_ecpm < it->ecpm())
              {
                profile_changed = true;

                inv_info.disappear_channel_ecpms.push_back(
                  ChannelECPM(size_it->first, *ch_it, it->ecpm()));
                continue;
              }
              else // it->ecpm() <= check_ecpm
              {
                size_it->second.erase(fit);
              }
            }

            *result_it = *ch_it;
            ++result_it;
          }

          if (result_it != channels.end())
          {
            channels.erase(result_it, channels.end());
          }

          // remove empty cells
          if(size_it->second.empty())
          {
            size_channels.erase(size_it);
          }

          if(channels.empty())
          {
            profile_changed = true;

            profile_writer.channel_price_ranges().erase(it++);
            continue;
          }
        }
      }

      ++it;
    }

    if(!size_channels.empty())
    {
      for(SizeChannelSet::const_iterator size_it = size_channels.begin();
          size_it != size_channels.end();
          ++size_it)
      {
        for(ChannelIdSet::const_iterator ch_it = size_it->second.begin();
            ch_it != size_it->second.end(); ++ch_it)
        {
          inv_info.appear_channel_ecpms.push_back(
            SizeChannel(size_it->first, *ch_it));
        }
      }

      // search cells with equal PK(country, tag, ecpm) and update
      for(std::list<ChInvByCMPCellWriter>::iterator
            it = profile_writer.channel_price_ranges().begin();
          it != profile_writer.channel_price_ranges().end();
          ++it)
      {
        if(inv_request_info.country_code.str() == (*it).country() &&
           check_ecpm == (*it).ecpm())
        {
          SizeChannelSet::iterator size_it =
            size_channels.find(Commons::ImmutableString((*it).tag_size()));

          if(size_it != size_channels.end())
          {
            (*it).channel_list().reserve(
              (*it).channel_list().size() + size_it->second.size());
            profile_changed = true;
            std::copy(
              size_it->second.begin(),
              size_it->second.end(),
              std::back_inserter((*it).channel_list()));

            size_channels.erase(size_it);
          }
        }
      }

      if(!size_channels.empty())
      {
        profile_changed = true;
      }

      for(SizeChannelSet::const_iterator size_it = size_channels.begin();
          size_it != size_channels.end();
          ++size_it)
      {
        ChInvByCMPCellWriter inv_cell_writer;
        inv_cell_writer.ecpm() = check_ecpm;
        inv_cell_writer.country() = inv_request_info.country_code.str();
        inv_cell_writer.tag_size() = size_it->first.str();
        inv_cell_writer.channel_list().reserve(size_it->second.size());
        std::copy(
          size_it->second.begin(),
          size_it->second.end(),
          std::back_inserter(inv_cell_writer.channel_list()));
        profile_writer.channel_price_ranges().push_back(
          std::move(inv_cell_writer));
      }
    }
  }

  bool
  fill_channel_typed_appears(
    bool& profile_changed,
    ChannelInventoryDayWriter::display_imp_channel_list_Container&
      res_imp_channel_list,
    ChannelInventoryDayWriter::display_imp_other_channel_list_Container&
      res_imp_other_channel_list,
    ChannelInventoryDayWriter::display_impop_no_imp_channel_list_Container&
      res_impop_no_imp_channel_list,
    InventoryActionProcessor::InventoryInfo::ChannelImpAppearInfo& appears,
    const ChannelIdVector& triggered_channels,
    const ChannelIdSet& impression_channels,
    bool impression)
    /*throw(PlainTypes::CorruptedStruct)*/
  {
    {
      // search channels that not present in any set
      ChannelInventoryDayWriter::display_imp_channel_list_Container::
        const_iterator imp_channel_list_it = res_imp_channel_list.begin();
      ChannelInventoryDayWriter::display_imp_other_channel_list_Container::
        const_iterator imp_other_channel_list_it = res_imp_other_channel_list.begin();
      ChannelInventoryDayWriter::display_impop_no_imp_channel_list_Container::
        const_iterator impop_no_imp_channel_list_it = res_impop_no_imp_channel_list.begin();

      for(auto ch_it = triggered_channels.begin();
          ch_it != triggered_channels.end(); ++ch_it)
      {
        while(imp_channel_list_it != res_imp_channel_list.end() &&
          *imp_channel_list_it < *ch_it)
        {
          ++imp_channel_list_it;
        }

        while(imp_other_channel_list_it != res_imp_other_channel_list.end() &&
          *imp_other_channel_list_it < *ch_it)
        {
          ++imp_other_channel_list_it;
        }

        while(impop_no_imp_channel_list_it != res_impop_no_imp_channel_list.end() &&
            *impop_no_imp_channel_list_it < *ch_it)
        {
          ++impop_no_imp_channel_list_it;
        }

        if((imp_channel_list_it == res_imp_channel_list.end() ||
             *imp_channel_list_it != *ch_it) &&
           (imp_other_channel_list_it == res_imp_other_channel_list.end() ||
             *imp_other_channel_list_it != *ch_it) &&
           (impop_no_imp_channel_list_it == res_impop_no_imp_channel_list.end() ||
             *impop_no_imp_channel_list_it != *ch_it))
        {
          appears.impop_appear_channels.push_back(*ch_it);
        }
      }
    }

    if(impression)
    {
      {
        // fill appears.imp_appear_channels and res_imp_channel_list
        const ChannelInventoryDayWriter::display_imp_channel_list_Container&
          imp_channel_list = res_imp_channel_list;
        //ChannelInventoryDayWriter::display_imp_channel_list_Container::
        //  const_iterator imp_channel_list_it = imp_channel_list.begin();

        ChannelIdVector imp_appear_channels;
        imp_appear_channels.reserve(impression_channels.size());

        std::set_difference(
          impression_channels.begin(),
          impression_channels.end(),
          imp_channel_list.begin(),
          imp_channel_list.end(),
          std::back_inserter(imp_appear_channels));

        if(!imp_appear_channels.empty())
        {
          ChannelInventoryDayWriter::display_imp_channel_list_Container
            new_imp_channel_list;
          new_imp_channel_list.reserve(
            imp_channel_list.size() + appears.imp_appear_channels.size());
          std::merge(
            imp_channel_list.begin(),
            imp_channel_list.end(),
            imp_appear_channels.begin(),
            imp_appear_channels.end(),
            std::back_inserter(new_imp_channel_list));
          res_imp_channel_list.swap(new_imp_channel_list);

          appears.imp_appear_channels.insert(
            appears.imp_appear_channels.end(),
            imp_appear_channels.begin(),
            imp_appear_channels.end());

          profile_changed = true;
        }
      }

      {
        const ChannelInventoryDayWriter::display_imp_other_channel_list_Container&
          imp_other_channel_list = res_imp_other_channel_list;
        //ChannelInventoryDayWriter::display_imp_other_channel_list_Container::
        //  const_iterator imp_other_channel_list_it = imp_other_channel_list.begin();

        ChannelInventoryDayWriter::display_imp_other_channel_list_Container
          new_imp_other_channel_list;
        new_imp_other_channel_list.reserve(
          imp_other_channel_list.size() + triggered_channels.size());

        ChannelIdVector impression_other_channels;
        impression_other_channels.reserve(triggered_channels.size());

        std::set_difference(
          triggered_channels.begin(),
          triggered_channels.end(),
          impression_channels.begin(),
          impression_channels.end(),
          std::back_inserter(impression_other_channels));

        ChannelIdVector imp_other_appear_channels;
        imp_other_appear_channels.reserve(impression_other_channels.size());

        std::set_difference(
          impression_other_channels.begin(),
          impression_other_channels.end(),
          imp_other_channel_list.begin(),
          imp_other_channel_list.end(),
          std::back_inserter(imp_other_appear_channels));

        if(!imp_other_appear_channels.empty())
        {
          ChannelInventoryDayWriter::display_imp_other_channel_list_Container
            new_imp_other_channel_list;
          new_imp_other_channel_list.reserve(
            imp_other_channel_list.size() + appears.imp_other_appear_channels.size());
          std::merge(
            imp_other_channel_list.begin(),
            imp_other_channel_list.end(),
            imp_other_appear_channels.begin(),
            imp_other_appear_channels.end(),
            std::back_inserter(new_imp_other_channel_list));
          res_imp_other_channel_list.swap(new_imp_other_channel_list);

          appears.imp_other_appear_channels.insert(
            appears.imp_other_appear_channels.end(),
            imp_other_appear_channels.begin(),
            imp_other_appear_channels.end());

          profile_changed = true;
        }
      }
    }
    else
    {
      const ChannelInventoryDayWriter::display_impop_no_imp_channel_list_Container&
        impop_no_imp_channel_list = res_impop_no_imp_channel_list;
      ChannelInventoryDayWriter::display_impop_no_imp_channel_list_Container
        new_impop_no_imp_channel_list;

      ChannelIdVector impop_no_imp_appear_channels;
      impop_no_imp_appear_channels.reserve(triggered_channels.size());

      std::set_difference(
        triggered_channels.begin(),
        triggered_channels.end(),
        impop_no_imp_channel_list.begin(),
        impop_no_imp_channel_list.end(),
        std::back_inserter(impop_no_imp_appear_channels));

      if(!impop_no_imp_appear_channels.empty())
      {
        ChannelInventoryDayWriter::display_impop_no_imp_channel_list_Container
          new_impop_no_imp_channel_list;
        new_impop_no_imp_channel_list.reserve(
          impop_no_imp_channel_list.size() +
          appears.impop_no_imp_appear_channels.size());
        std::merge(
          impop_no_imp_channel_list.begin(),
          impop_no_imp_channel_list.end(),
          impop_no_imp_appear_channels.begin(),
          impop_no_imp_appear_channels.end(),
          std::back_inserter(new_impop_no_imp_channel_list));
        res_impop_no_imp_channel_list.swap(new_impop_no_imp_channel_list);

        appears.impop_no_imp_appear_channels.insert(
          appears.impop_no_imp_appear_channels.end(),
          impop_no_imp_appear_channels.begin(),
          impop_no_imp_appear_channels.end());

        profile_changed = true;
      }
    }

    return true;
  }

  void
  fill_channel_appears(
    bool& profile_changed,
    InventoryActionProcessor::InventoryInfo& inv_info,
    ChannelInventoryDayWriter& profile_writer,
    const MatchRequestProcessor::MatchInfo& inv_request_info)
    /*throw(PlainTypes::CorruptedStruct)*/
  {
    {
      // fill active_appear_channels
      ChannelInventoryDayWriter::active_channel_list_Container&
        active_channel_list = profile_writer.active_channel_list();

      assert(inv_info.active_appear_channels.empty());
      if (inv_request_info.triggered_expression_channels.present())
      {
        std::set_difference(
          inv_request_info.triggered_expression_channels->begin(),
          inv_request_info.triggered_expression_channels->end(),
          active_channel_list.begin(),
          active_channel_list.end(),
          std::back_inserter(inv_info.active_appear_channels));
      }

      if(!inv_info.active_appear_channels.empty())
      {
        ChannelInventoryDayWriter::active_channel_list_Container
          new_active_channel_list;
        active_channel_list.reserve(
          active_channel_list.size() + inv_info.active_appear_channels.size());
        std::merge(
          active_channel_list.begin(),
          active_channel_list.end(),
          inv_info.active_appear_channels.begin(),
          inv_info.active_appear_channels.end(),
          std::back_inserter(new_active_channel_list));
        active_channel_list.swap(new_active_channel_list);
        profile_changed = true;
      }
    }

    if(!inv_info.active_appear_channels.empty())
    {
      // used, that active channels always is subset of total channels
      // and as result: total appear is subset of active appear
      // fill total_appear_channels
      ChannelInventoryDayWriter::total_channel_list_Container&
        total_channel_list = profile_writer.total_channel_list();

      assert(inv_info.total_appear_channels.empty());

      std::set_difference(
        inv_info.active_appear_channels.begin(),
        inv_info.active_appear_channels.end(),
        total_channel_list.begin(),
        total_channel_list.end(),
        std::back_inserter(inv_info.total_appear_channels));

      if(!inv_info.total_appear_channels.empty())
      {
        ChannelInventoryDayWriter::total_channel_list_Container
          new_total_channel_list;
        total_channel_list.reserve(
          total_channel_list.size() + inv_info.total_appear_channels.size());
        std::merge(
          total_channel_list.begin(),
          total_channel_list.end(),
          inv_info.total_appear_channels.begin(),
          inv_info.total_appear_channels.end(),
          std::back_inserter(new_total_channel_list));
        total_channel_list.swap(new_total_channel_list);
        profile_changed = true;
      }
    }

    ChannelIdVector triggered_expr_holder;
    const ChannelIdVector& triggered_expression_channels =
      inv_request_info.triggered_expression_channels.present() ?
      *inv_request_info.triggered_expression_channels : triggered_expr_holder;

    if(!inv_request_info.tag_size.empty()) // impop
    {
      fill_channel_typed_appears(
        profile_changed,
        profile_writer.display_imp_channel_list(),
        profile_writer.display_imp_other_channel_list(),
        profile_writer.display_impop_no_imp_channel_list(),
        inv_info.display_appears,
        triggered_expression_channels,
        inv_request_info.display_ad.present() ?
          inv_request_info.display_ad->imp_channels : ChannelIdSet(),
        inv_request_info.display_ad.present() ||
          !inv_request_info.text_ads.empty());

      // TO OPTIMIZE
      MatchRequestProcessor::MatchInfo::AdBidSlotList::
        const_iterator text_ad_it = inv_request_info.text_ads.begin();

      for(unsigned long text_i = 0;
          text_i < inv_request_info.max_text_ads; ++text_i)
      {
        fill_channel_typed_appears(
          profile_changed,
          profile_writer.text_imp_channel_list(),
          profile_writer.text_imp_other_channel_list(),
          profile_writer.text_impop_no_imp_channel_list(),
          inv_info.text_appears,
          triggered_expression_channels,
          text_ad_it != inv_request_info.text_ads.end() ?
            text_ad_it->imp_channels : ChannelIdSet(),
          inv_request_info.display_ad.present() ||
            text_ad_it != inv_request_info.text_ads.end());

        if(text_ad_it != inv_request_info.text_ads.end())
        {
          ++text_ad_it;
        }
      }
    }
  }

  void
  fill_inv_info_by_day(
    bool& profile_changed,
    const MatchRequestProcessor::MatchInfo& inv_request_info,
    ChannelInventoryDayWriter& profile_writer,
    InventoryActionProcessor::InventoryInfo& inv_info)
    /*throw(eh::Exception, PlainTypes::CorruptedStruct)*/
  {
    fill_channel_appears(
      profile_changed,
      inv_info,
      profile_writer,
      inv_request_info);

    if(!inv_request_info.tag_size.empty())
    {
      fill_channel_ecpms(
        profile_changed,
        inv_info,
        profile_writer,
        inv_request_info);
    }
    else
    {
      inv_info.impop_ecpm = 0;
    }
  }

  void
  clear_expired_days(
    bool& profile_changed,
    UserChannelInventoryProfileWriter& profile_writer,
    const Generics::Time& date,
    const Generics::Time& days_to_keep)
    noexcept
  {
    typedef UserChannelInventoryProfileWriter::days_Container DayList;

    DayList::iterator day_erase_it =
      std::upper_bound(
        profile_writer.days().begin(),
        profile_writer.days().end(),
        date - days_to_keep,
        DateCompare());

    if(day_erase_it != profile_writer.days().begin())
    {
      profile_changed = true;

      profile_writer.days().erase(
        profile_writer.days().begin(),
        day_erase_it);
    }
  }

  void
  fill_inv_info(
    bool& profile_changed,
    UserChannelInventoryProfileWriter& profile_writer,
    bool& delegate_inventory_processing,
    InventoryActionProcessor::InventoryInfo& inv_info,
    const MatchRequestProcessor::MatchInfo& inv_request_info,
    const Generics::Time& days_to_keep)
    /*throw(eh::Exception, PlainTypes::CorruptedStruct)*/
  {
    delegate_inventory_processing = false;

    const Generics::Time request_date =
      inv_request_info.placement_colo_time.get_gm_time().get_date();

    if(profile_writer.days().empty() ||
       request_date > Generics::Time(
         profile_writer.days().back().date()).get_gm_time().get_date() - days_to_keep)
    {
      if(profile_writer.last_request_time() < request_date.tv_sec)
      {
        profile_changed = true;
        profile_writer.last_request_time() = request_date.tv_sec;
      }

      if(profile_writer.last_daily_processing_time() < request_date.tv_sec &&
         inv_request_info.triggered_expression_channels.present())
      {
        profile_changed = true;
        profile_writer.last_daily_processing_time() =
          inv_request_info.placement_colo_time.tv_sec;
      }

      ChannelInventoryDayWriter& new_writer = get_inv_day(
        profile_changed,
        profile_writer,
        request_date);

      fill_inv_info_by_day(
        profile_changed,
        inv_request_info,
        new_writer,
        inv_info);
      delegate_inventory_processing = true;
    }

    clear_expired_days(
      profile_changed,
      profile_writer,
      request_date,
      days_to_keep);
  }

  /* UserInventoryInfoContainer */
  UserInventoryInfoContainer::UserInventoryInfoContainer(
    Logging::Logger* logger,
    const Generics::Time& days_to_keep,
    InventoryActionProcessor* inv_processor,
    ColoReachProcessor* colo_reach_processor,
    bool adrequest_anonymize,
    unsigned long common_chunks_number,
    const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
    const char* inv_prefix,
    ProfilingCommons::ProfileMapFactory::Cache* /*cache*/,
    const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      days_to_keep_(days_to_keep),
      adrequest_anonymize_(adrequest_anonymize),
      //expire_time_(expire_time),
      inventory_processor_(ReferenceCounting::add_ref(inv_processor)),
      colo_reach_processor_(ReferenceCounting::add_ref(colo_reach_processor))
  {
    static const char* FUN = "UserInventoryInfoContainer::UserInventoryInfoContainer()";

    try
    {
      typedef AdServer::ProfilingCommons::OptionalProfileAdapter<UserChannelInventoryProfileAdapter>
        AdaptUserChannelInventoryProfile;

      user_map_ = AdServer::ProfilingCommons::ProfileMapFactory::
        open_chunked_map<
          UserId,
          AdServer::ProfilingCommons::UserIdAccessor,
          unsigned long (*)(const Generics::Uuid& uuid),
          AdaptUserChannelInventoryProfile>(
            common_chunks_number,
            chunk_folders,
            inv_prefix,
            user_level_map_traits,
            *this,
            Generics::ActiveObjectCallback_var(
              new Logging::ActiveObjectCallbackImpl(
                logger_,
                "UserInventoryInfoContainer",
                "ExpressionMatcher",
                "ADS-IMPL-4024")),
            AdServer::Commons::uuid_distribution_hash);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init UserInventoryInfoMap. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  UserInventoryInfoContainer::~UserInventoryInfoContainer() noexcept
  {}

  Generics::ConstSmartMemBuf_var
  UserInventoryInfoContainer::get_profile(
    const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserInventoryInfoContainer::get_profile()";

    try
    {
      return user_map_->get_profile(user_id);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get profile. Caught eh::Exception: " <<
        e.what();
      throw Exception(ostr);
    }
  }

  void
  UserInventoryInfoContainer::process_match_request(
    const MatchInfo& request_info)
    /*throw(MatchRequestProcessor::Exception)*/
  {
    static const char* FUN = "UserInventoryInfoContainer::process_match_request()";

    bool delegate_inventory_processing = false;
    InventoryActionProcessor::InventoryInfo inv_info;
    ColoReachInfoList gmt_colo_reach_info_list;
    ColoReachInfoList isp_colo_reach_info_list;

    try
    {
      if(!request_info.user_id.is_null())
      {
        process_request_trans_(
          delegate_inventory_processing,
          inv_info,
          gmt_colo_reach_info_list,
          isp_colo_reach_info_list,
          request_info);
      }
      else
      {
        delegate_inventory_processing = init_inv_info_(inv_info, request_info);
      }

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": Process inventory request: " << std::endl <<
          "Request info: " << std::endl;
        request_info.print(ostr, "  ");
        ostr << std::endl << "Inventory info: " << std::endl;
        inv_info.print(ostr, "  ");

        logger_->log(ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_INVENTORY_CONTAINER);
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception "
        "on processing request transaction: " << e.what();
      throw MatchRequestProcessor::Exception(ostr);
    }

    if(delegate_inventory_processing)
    {
      try
      {
        inventory_processor_->process_request(inv_info);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception "
          "on request processing delegate: " << e.what();
        throw MatchRequestProcessor::Exception(ostr);
      }
    }

    if(colo_reach_processor_.in())
    {
      try
      {
        for(ColoReachInfoList::const_iterator it = gmt_colo_reach_info_list.begin();
            it != gmt_colo_reach_info_list.end(); ++it)
        {
          colo_reach_processor_->process_gmt_colo_reach(*it);
        }

        for(ColoReachInfoList::const_iterator it = isp_colo_reach_info_list.begin();
            it != isp_colo_reach_info_list.end(); ++it)
        {
          colo_reach_processor_->process_isp_colo_reach(*it);
        }
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception "
          "on request processing delegate: " << e.what();
        throw MatchRequestProcessor::Exception(ostr);
      }
    }
  }

  bool
  UserInventoryInfoContainer::get_last_daily_processing_time(
    const AdServer::Commons::UserId& user_id,
    Generics::Time& last_daily_processing_time)
    /*throw(Exception)*/
  {
    static const char* FUN =
      "UserInventoryInfoContainer::get_last_daily_processing_time()";

    try
    {
      Generics::ConstSmartMemBuf_var mem_buf = get_profile(user_id);

      if(mem_buf.in())
      {
        UserChannelInventoryProfileReader profile_reader(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        last_daily_processing_time =
          Generics::Time(profile_reader.last_daily_processing_time());

        return last_daily_processing_time != Generics::Time::ZERO;
      }

      return false;
    }
    catch(const eh::Exception& ex)
    {
      try
      {
        // corrupted profile - remove it
        user_map_->remove_profile(user_id);
      }
      catch(...)
      {}

      Stream::Error ostr;
      ostr << FUN << ": for user_id = " << user_id.to_string() <<
        "' caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInventoryInfoContainer::process_user(
    const InventoryDailyMatchInfo& inv_daily_match_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserInventoryInfoContainer::process_user()";

    bool delegate_processing;
    InventoryActionProcessor::InventoryUserInfo inv_info;

    process_user_trans_(delegate_processing, inv_info, inv_daily_match_info);

    if(delegate_processing)
    {
      try
      {
        inventory_processor_->process_user(inv_info);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception "
          "on user daily processing delegate: " << e.what();
        throw Exception(ostr);
      }
    }
  }

  void
  UserInventoryInfoContainer::clear_expired_users()
    /*throw(Exception)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    user_map_->clear_expired(now - expire_time_);
  }

  void
  UserInventoryInfoContainer::all_users(UserIdList& users)
    /*throw(Exception)*/
  {
    user_map_->copy_keys(users);
  }


  void
  UserInventoryInfoContainer::save_profile_(
    const AdServer::Commons::UserId& user_id,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& time)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserInventoryInfoContainer::save_profile()";

    try
    {
      user_map_->save_profile(user_id, profile, time);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't save profile. Caught eh::Exception: " <<
        e.what();
      throw Exception(ostr);
    }
  }

  void
  UserInventoryInfoContainer::process_request_trans_(
    bool& delegate_inventory_processing,
    InventoryActionProcessor::InventoryInfo& inv_info,
    ColoReachInfoList& gmt_colo_reach_info_list,
    ColoReachInfoList& isp_colo_reach_info_list,
    const MatchInfo& request_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserInventoryInfoContainer::process_request_trans_()";

    delegate_inventory_processing = false;

    ColoReachProcessor::ColoReachInfo gmt_colo_reach_info;
    ColoReachProcessor::ColoReachInfo isp_colo_reach_info;

    UserInventoryInfoMap::Transaction_var transaction;

    try
    {
      Generics::ConstSmartMemBuf_var mem_buf;

      try
      {
        transaction = user_map_->get_transaction(request_info.user_id);

        mem_buf = transaction->get_profile();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": on read, for user_id = " << request_info.user_id.to_string() <<
          "' caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }

      UserChannelInventoryProfileWriter profile_writer;

      const Generics::Time& time = request_info.time;
      const Generics::Time date(Algs::round_to_day(time));

      const Generics::Time& isp_time = request_info.isp_time;
      const Generics::Time isp_date(Algs::round_to_day(isp_time));

      bool save_colo = false;
      bool colo_appeared = true;
      bool isp_colo_appeared = true;
      bool colo_ad_appeared = false;
      bool isp_colo_ad_appeared = false;
      bool colo_merge_appeared = false;
      bool isp_colo_merge_appeared = false;

      if(mem_buf.in())
      {
        UserChannelInventoryProfileReader profile_reader(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        profile_writer.init(mem_buf->membuf().data(),
          mem_buf->membuf().size());

        Generics::Time old_isp_time;
        const bool old_isp_time_found =
          get_time_by_id(
            profile_reader.isp_colo_create_time().begin(),
            profile_reader.isp_colo_create_time().end(),
            request_info.colo_id,
            old_isp_time);

        const Generics::Time old_gmt_time =
          Generics::Time(profile_reader.create_time());

        const bool changed_create_time = (time < old_gmt_time);
        // changed_create_time: revert and resave all data,
        //   with possible updated TZ for passed colo_id, insert new colo_id.
        // changed_isp_time only - change TZ only for passed and known id
        // If none of create_time or isp_create_time changed for passed id.
        //   (possible) insert new colo id, update appearance

        const bool changed_isp_create_time = old_isp_time_found &&
          (isp_time < old_isp_time);

        if(changed_isp_create_time || changed_create_time)
        {
          save_colo = true;

          gmt_colo_reach_info_list.push_back(ColoReachProcessor::ColoReachInfo());

          ColoReachProcessor::ColoReachInfo& revert_gmt_colo_reach_info =
            gmt_colo_reach_info_list.back();

          revert_gmt_colo_reach_info.create_time =
            Generics::Time(profile_reader.create_time());
          revert_gmt_colo_reach_info.household = false;

          ColoReachProcessor::ColoReachInfo
            revert_tmp_collector, resave_tmp_collector;

          // collect old records
          collect_all_appearance(
            revert_tmp_collector.colocations,
            profile_reader.isp_colo_appears().begin(),
            profile_reader.isp_colo_appears().end(),
            -1,
            BaseAppearanceIdGetter());

          collect_all_appearance(
            revert_tmp_collector.ad_colocations,
            profile_reader.isp_colo_ad_appears().begin(),
            profile_reader.isp_colo_ad_appears().end(),
            -1,
            BaseAppearanceIdGetter());

          collect_all_appearance(
            revert_tmp_collector.merge_colocations,
            profile_reader.isp_colo_merge_appears().begin(),
            profile_reader.isp_colo_merge_appears().end(),
            -1,
            BaseAppearanceIdGetter());

          // collect new records
          collect_all_appearance(
            resave_tmp_collector.colocations,
            profile_reader.isp_colo_appears().begin(),
            profile_reader.isp_colo_appears().end(),
            1,
            BaseAppearanceIdGetter());

          collect_all_appearance(
            resave_tmp_collector.ad_colocations,
            profile_reader.isp_colo_ad_appears().begin(),
            profile_reader.isp_colo_ad_appears().end(),
            1,
            BaseAppearanceIdGetter());

          collect_all_appearance(
            resave_tmp_collector.merge_colocations,
            profile_reader.isp_colo_merge_appears().begin(),
            profile_reader.isp_colo_merge_appears().end(),
            1,
            BaseAppearanceIdGetter());

          // resave all appearances
          if(changed_create_time)
          {
            profile_writer.create_time() = time.tv_sec;

            collect_all_appearance(
              revert_gmt_colo_reach_info.colocations,
              profile_reader.colo_appears().begin(),
              profile_reader.colo_appears().end(),
              -1,
              BaseAppearanceIdGetter());

            collect_all_appearance(
              revert_gmt_colo_reach_info.ad_colocations,
              profile_reader.colo_ad_appears().begin(),
              profile_reader.colo_ad_appears().end(),
              -1,
              BaseAppearanceIdGetter());

            collect_all_appearance(
              revert_gmt_colo_reach_info.merge_colocations,
              profile_reader.colo_merge_appears().begin(),
              profile_reader.colo_merge_appears().end(),
              -1,
              BaseAppearanceIdGetter());

            collect_all_appearance(
              gmt_colo_reach_info.colocations,
              profile_reader.colo_appears().begin(),
              profile_reader.colo_appears().end(),
              1,
              BaseAppearanceIdGetter());

            collect_all_appearance(
              gmt_colo_reach_info.ad_colocations,
              profile_reader.colo_ad_appears().begin(),
              profile_reader.colo_ad_appears().end(),
              1,
              BaseAppearanceIdGetter());

            collect_all_appearance(
              gmt_colo_reach_info.merge_colocations,
              profile_reader.colo_merge_appears().begin(),
              profile_reader.colo_merge_appears().end(),
              1,
              BaseAppearanceIdGetter());

            update_id_time_list(
              profile_writer.isp_colo_create_time(),
              request_info.colo_id,
              old_gmt_time,
              time,
              isp_time,
              BaseIdTimeLess(),
              BaseInsertIdTimeAdapter<ColoCreateTimeWriter>());

            // revert and resave  records for all colo_id
            split_colo_reach_info_by_id(
              isp_colo_reach_info_list,
              revert_tmp_collector,
              request_info.colo_id,
              false, // household
              profile_reader.isp_colo_create_time().begin(),
              profile_reader.isp_colo_create_time().end(),
              false);

            split_colo_reach_info_by_id(
              isp_colo_reach_info_list,
              resave_tmp_collector,
              request_info.colo_id,
              false, // household
              profile_writer.isp_colo_create_time().begin(),
              profile_writer.isp_colo_create_time().end(),
              false);
          }

          if(changed_isp_create_time && !changed_create_time)
          {
            // Here we revert and resave only data for current colo_id
            // Only TZ for colo_id changed,
            update_id_time_list(
              profile_writer.isp_colo_create_time(),
              request_info.colo_id,
              old_gmt_time,
              time,
              isp_time,
              BaseIdTimeLess(),
              BaseInsertIdTimeAdapter<ColoCreateTimeWriter>());

            // revert and resave  records for all colo_id
            split_colo_reach_info_by_id(
              isp_colo_reach_info_list,
              revert_tmp_collector,
              request_info.colo_id,
              false, // household
              profile_reader.isp_colo_create_time().begin(),
              profile_reader.isp_colo_create_time().end(),
              true);

            split_colo_reach_info_by_id(
              isp_colo_reach_info_list,
              resave_tmp_collector,
              request_info.colo_id,
              false, // household
              profile_writer.isp_colo_create_time().begin(),
              profile_writer.isp_colo_create_time().end(),
              true);
          }
        }

        if(profile_reader.create_time() == 0)
        {
          profile_writer.create_time() = date.tv_sec;
        }

        {
          // Possible add new colo_id in isp_colo_create_time
          Generics::Time new_isp_time;
          const bool new_isp_time_found =
            get_time_by_id(
              profile_writer.isp_colo_create_time().begin(),
              profile_writer.isp_colo_create_time().end(),
              request_info.colo_id,
              new_isp_time);

          if (!new_isp_time_found || new_isp_time == Generics::Time::ZERO)
          {
            update_id_time_list(
              profile_writer.isp_colo_create_time(),
              request_info.colo_id,
              old_gmt_time,
              time,
              isp_time,
              BaseIdTimeLess(),
              BaseInsertIdTimeAdapter<ColoCreateTimeWriter>());
          }
        }

        // current request
        colo_appeared = collect_appearance(
          gmt_colo_reach_info.colocations,
          profile_reader.colo_appears().begin(),
          profile_reader.colo_appears().end(),
          request_info.colo_id,
          date,
          BaseAppearanceLess());

        isp_colo_appeared = collect_appearance(
          isp_colo_reach_info.colocations,
          profile_reader.isp_colo_appears().begin(),
          profile_reader.isp_colo_appears().end(),
          request_info.colo_id,
          isp_date,
          BaseAppearanceLess());

        if(request_info.ad_request)
        {
          // ad request (excluding inventory requests)
          colo_ad_appeared = collect_appearance(
            gmt_colo_reach_info.ad_colocations,
            profile_reader.colo_ad_appears().begin(),
            profile_reader.colo_ad_appears().end(),
            request_info.colo_id,
            date,
            BaseAppearanceLess());

          isp_colo_ad_appeared = collect_appearance(
            isp_colo_reach_info.ad_colocations,
            profile_reader.isp_colo_ad_appears().begin(),
            profile_reader.isp_colo_ad_appears().end(),
            request_info.colo_id,
            isp_date,
            BaseAppearanceLess());
        }

        if(request_info.merge_request)
        {
          // merge request
          colo_merge_appeared = collect_appearance(
            gmt_colo_reach_info.merge_colocations,
            profile_reader.colo_merge_appears().begin(),
            profile_reader.colo_merge_appears().end(),
            request_info.colo_id,
            date,
            BaseAppearanceLess());

          isp_colo_merge_appeared = collect_appearance(
            isp_colo_reach_info.merge_colocations,
            profile_reader.isp_colo_merge_appears().begin(),
            profile_reader.isp_colo_merge_appears().end(),
            request_info.colo_id,
            isp_date,
            BaseAppearanceLess());
        }
      }
      else
      {
        // membuf.in() == 0, create new profile
        init_profile_writer(profile_writer);

        profile_writer.create_time() = time.tv_sec;
        update_id_time_list(
          profile_writer.isp_colo_create_time(),
          request_info.colo_id,
          time,
          time,
          isp_time,
          BaseIdTimeLess(),
          BaseInsertIdTimeAdapter<ColoCreateTimeWriter>());

        add_total_appearance(
          gmt_colo_reach_info.colocations,
          request_info.colo_id,
          date);

        add_total_appearance(
          isp_colo_reach_info.colocations,
          request_info.colo_id,
          isp_date);

        if(request_info.ad_request)
        {
          add_total_appearance(
            gmt_colo_reach_info.ad_colocations,
            request_info.colo_id,
            date);

          add_total_appearance(
            isp_colo_reach_info.ad_colocations,
            request_info.colo_id,
            isp_date);

          colo_ad_appeared = true;
          isp_colo_ad_appeared = true;
        }

        if(request_info.merge_request)
        {
          add_total_appearance(
            gmt_colo_reach_info.merge_colocations,
            request_info.colo_id,
            date);

          add_total_appearance(
            isp_colo_reach_info.merge_colocations,
            request_info.colo_id,
            isp_date);

          colo_merge_appeared = true;
          isp_colo_merge_appeared = true;
        }
      }

      delegate_inventory_processing = init_inv_info_(
        inv_info, request_info);

      bool profile_changed = false;

      if(!adrequest_anonymize_ || request_info.tag_size.empty())
      {

        fill_inv_info(
          profile_changed,
          profile_writer,
          delegate_inventory_processing,
          inv_info,
          request_info,
          days_to_keep_);
      }

      if(save_colo ||
         colo_appeared ||
         isp_colo_appeared ||
         colo_ad_appeared ||
         isp_colo_ad_appeared ||
         colo_merge_appeared ||
         isp_colo_merge_appeared)
      {
        profile_changed = true;

        gmt_colo_reach_info.create_time =
          Generics::Time(profile_writer.create_time());
        gmt_colo_reach_info.household = false;

        Generics::Time isp_create_time;
        const bool found_id_profile_writer = get_time_by_id(
          profile_writer.isp_colo_create_time().begin(),
          profile_writer.isp_colo_create_time().end(),
          request_info.colo_id,
          isp_create_time);
        const Generics::Time isp_create_date = Algs::round_to_day(isp_create_time);

        if (!found_id_profile_writer)
        {
          // smth goes wrong. at this point in profile writer
          //   isp_colo_create_time must contain element with request_info.colo_id
          Stream::Error ostr;
          ostr << FUN << "request_info.colo_id: " << request_info.colo_id <<
            " not found in profile_writer.isp_colo_create_time";
          throw Exception(ostr);
        }

        isp_colo_reach_info.create_time = isp_create_date;
        isp_colo_reach_info.household = false;

        gmt_colo_reach_info_list.push_back(gmt_colo_reach_info);
        isp_colo_reach_info_list.push_back(isp_colo_reach_info);

        if(colo_appeared)
        {
          update_appearance(
            profile_writer.colo_appears(),
            request_info.colo_id,
            date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<ColoIdAppearanceWriter>());
        }

        if(isp_colo_appeared)
        {
          update_appearance(
            profile_writer.isp_colo_appears(),
            request_info.colo_id,
            isp_date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<ColoIdAppearanceWriter>());
        }

        if(colo_ad_appeared)
        {
          update_appearance(
            profile_writer.colo_ad_appears(),
            request_info.colo_id,
            date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<ColoIdAppearanceWriter>());
        }

        if(isp_colo_ad_appeared)
        {
          update_appearance(
            profile_writer.isp_colo_ad_appears(),
            request_info.colo_id,
            isp_date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<ColoIdAppearanceWriter>());
        }

        if(colo_merge_appeared)
        {
          update_appearance(
            profile_writer.colo_merge_appears(),
            request_info.colo_id,
            date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<ColoIdAppearanceWriter>());
        }

        if(isp_colo_merge_appeared)
        {
          update_appearance(
            profile_writer.isp_colo_merge_appears(),
            request_info.colo_id,
            isp_date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<ColoIdAppearanceWriter>());
        }
      }

      // save profile
      if(profile_changed)
      {
        try
        {
          Generics::SmartMemBuf_var new_mem_buf(
            new Generics::SmartMemBuf(profile_writer.size()));

          profile_writer.save(
            new_mem_buf->membuf().data(), new_mem_buf->membuf().size());

          transaction->save_profile(
            Generics::transfer_membuf(new_mem_buf),
            request_info.time);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": on save, caught eh::Exception: " << ex.what();
          throw Exception(ostr);
        }
      }
    }
    catch(const PlainTypes::CorruptedStruct& ex)
    {
      transaction->remove_profile();

      Stream::Error ostr;
      ostr << FUN << ": Caught PlainTypes::CorruptedStruct: " << ex.what();
      throw Exception(ostr);
    }
  }

  void UserInventoryInfoContainer::process_user_trans_(
    bool& delegate_processing,
    InventoryActionProcessor::InventoryUserInfo& inv_info,
    const InventoryDailyMatchInfo& inv_daily_match_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserInventoryInfoContainer::process_user_trans_()";

    typedef ChannelInventoryDayWriter::total_channel_list_Container
      ChannelIdWriteList;

    delegate_processing = false;

    try
    {
      UserInventoryInfoMap::Transaction_var transaction;

      Generics::ConstSmartMemBuf_var mem_buf;

      try
      {
        transaction = user_map_->get_transaction(inv_daily_match_info.user_id);

        mem_buf = transaction->get_profile();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": on read, for user_id = " << inv_daily_match_info.user_id.to_string() <<
          "' caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }

      if(mem_buf.in())
      {
        UserChannelInventoryProfileReader profile_reader(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        const Generics::Time last_daily_processing_time =
          Generics::Time(profile_reader.last_daily_processing_time());

        const Generics::Time processing_date =
          Generics::Time(inv_daily_match_info.time.get_gm_time().get_date());

        if(last_daily_processing_time < processing_date)
        {
          UserChannelInventoryProfileWriter profile_writer(
            mem_buf->membuf().data(),
            mem_buf->membuf().size());

          profile_writer.last_daily_processing_time() =
            processing_date.tv_sec;

          bool unused_profile_changed = false;

          ChannelInventoryDayWriter& day_writer = get_inv_day(
            unused_profile_changed,
            profile_writer,
            processing_date);

          ChannelIdVector total_appear_expression_channels;

          std::set_difference(
            inv_daily_match_info.triggered_expression_channels.begin(),
            inv_daily_match_info.triggered_expression_channels.end(),
            day_writer.total_channel_list().begin(),
            day_writer.total_channel_list().end(),
            std::back_inserter(total_appear_expression_channels));

          ChannelIdWriteList new_total_channel_list;

          std::merge(
            total_appear_expression_channels.begin(),
            total_appear_expression_channels.end(),
            day_writer.total_channel_list().begin(),
            day_writer.total_channel_list().end(),
            std::back_inserter(new_total_channel_list));

          day_writer.total_channel_list().swap(new_total_channel_list);

          clear_expired_days(
            unused_profile_changed,
            profile_writer,
            processing_date,
            days_to_keep_);

          if(!total_appear_expression_channels.empty())
          {
            delegate_processing = true;

            inv_info.time = processing_date;
            inv_info.placement_colo_time = processing_date;
            inv_info.colo_id = inv_daily_match_info.colo_id;

            inv_info.total_appear_channels.swap(
              total_appear_expression_channels);
          }

          Generics::Time last_request_time =
            Generics::Time(profile_reader.last_request_time()) +
              Generics::Time::ONE_DAY;

          try
          {
            /* save profile */
            Generics::SmartMemBuf_var new_mem_buf(
              new Generics::SmartMemBuf(profile_writer.size()));

            profile_writer.save(
              new_mem_buf->membuf().data(), new_mem_buf->membuf().size());

            // don't change profile saving time (last_request_time)
            transaction->save_profile(
              Generics::transfer_membuf(new_mem_buf),
              last_request_time);
          }
          catch(const eh::Exception& ex)
          {
            Stream::Error ostr;
            ostr << FUN << ": on save, caught eh::Exception: " << ex.what();
            throw Exception(ostr);
          }
        } // last_daily_processing_time < processing_date
      }
    }
    catch(const PlainTypes::CorruptedStruct& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught PlainTypes::CorruptedStruct: " << ex.what();
      throw Exception(ostr);
    }
  }

  bool
  UserInventoryInfoContainer::init_inv_info_(
    InventoryActionProcessor::InventoryInfo& inv_info,
    const MatchInfo& request_info)
    noexcept
  {
    inv_info.time = request_info.time;
    inv_info.isp_time = request_info.isp_time;
    inv_info.placement_colo_time = request_info.placement_colo_time;
    inv_info.colo_id = request_info.colo_id;
    inv_info.country_code = request_info.country_code;
    inv_info.sizes = request_info.sizes;
    inv_info.auction_type = request_info.auction_type;

    ChannelIdVector triggered_expr_holder;
    const ChannelIdVector& triggered_expression_channels =
      request_info.triggered_expression_channels.present() ?
      *request_info.triggered_expression_channels : triggered_expr_holder;

    if(!request_info.tag_size.empty())
    {
      RevenueDecimal sum_request_revenue = RevenueDecimal::ZERO;

      if(request_info.display_ad.present())
      {
        try
        {
          sum_request_revenue = request_info.display_ad->avg_revenue;
        }
        catch(...)
        {
          assert(0);
        }
      }
      else
      {
        for(MatchInfo::AdBidSlotList::
              const_iterator text_ad_it = request_info.text_ads.begin();
            text_ad_it != request_info.text_ads.end();
            ++text_ad_it)
        {
          try
          {
            sum_request_revenue += text_ad_it->avg_revenue;
          }
          catch(...)
          {
            assert(0);
          }
        }
      }

      // impression opportunity
      if(request_info.display_ad.present())
      {
        inv_info.display_imps.imp_channels.reserve(request_info.display_ad->imp_channels.size());

        // selected display ad
        for(ChannelIdSet::const_iterator imp_ch_it =
              request_info.display_ad->imp_channels.begin();
            imp_ch_it != request_info.display_ad->imp_channels.end();
            ++imp_ch_it)
        {
          inv_info.display_imps.imp_channels.emplace_back(
            *imp_ch_it,
            request_info.display_ad->avg_revenue);
        }

        inv_info.display_imps.imp_other_channels.reserve(triggered_expression_channels.size());
        inv_info.text_imps.imp_other_channels.reserve(triggered_expression_channels.size());

        for(auto ch_it = triggered_expression_channels.begin();
            ch_it != triggered_expression_channels.end(); ++ch_it)
        {
          if(request_info.display_ad->imp_channels.find(*ch_it) ==
             request_info.display_ad->imp_channels.end())
          {
            inv_info.display_imps.imp_other_channels.emplace_back(
              *ch_it,
              InventoryActionProcessor::InventoryInfo::ChannelImpCounter(
                 1, sum_request_revenue));
          }

          if(request_info.max_text_ads != 0)
          {
            inv_info.text_imps.imp_other_channels.emplace_back(
              *ch_it,
              InventoryActionProcessor::InventoryInfo::ChannelImpCounter(
                request_info.max_text_ads,
                request_info.display_ad->avg_revenue));
          }
        }
      }
      else if(!request_info.text_ads.empty())
      {
        inv_info.display_imps.imp_other_channels.reserve(triggered_expression_channels.size());
        inv_info.text_imps.imp_other_channels.reserve(triggered_expression_channels.size());

        // text ads selected
        for(auto ch_it = triggered_expression_channels.begin();
            ch_it != triggered_expression_channels.end(); ++ch_it)
        {
          inv_info.display_imps.imp_other_channels.emplace_back(
            *ch_it,
            InventoryActionProcessor::InventoryInfo::ChannelImpCounter(
              1, sum_request_revenue));
        }

        InventoryActionProcessor::InventoryInfo::
          ChannelImpCounterMap text_imp_channels;

        for(MatchInfo::AdBidSlotList::
              const_iterator text_ad_it = request_info.text_ads.begin();
            text_ad_it != request_info.text_ads.end();
            ++text_ad_it)
        {
          for(ChannelIdSet::const_iterator imp_ch_it =
                text_ad_it->imp_channels.begin();
              imp_ch_it != text_ad_it->imp_channels.end();
              ++imp_ch_it)
          {
            text_imp_channels[*imp_ch_it] +=
              InventoryActionProcessor::InventoryInfo::ChannelImpCounter(
                1, text_ad_it->avg_revenue);
          }
        }

        inv_info.text_imps.imp_channels.reserve(text_imp_channels.size());

        for (auto ch_cnt_it = text_imp_channels.begin();
             ch_cnt_it != text_imp_channels.end(); ++ch_cnt_it)
        {
          inv_info.text_imps.imp_channels.emplace_back(
            ch_cnt_it->first,
            ch_cnt_it->second.revenue);
        }

        const unsigned long text_ads_imps = request_info.text_ads.size();
        inv_info.text_imps.impop_no_imp_channels.reserve(triggered_expression_channels.size());

        for(auto ch_it = triggered_expression_channels.begin();
            ch_it != triggered_expression_channels.end(); ++ch_it)
        {
          const auto ch_cnt_it = text_imp_channels.find(*ch_it);

          const unsigned long imps = (
            ch_cnt_it != text_imp_channels.end() ?
            ch_cnt_it->second.imps : 0);

          const unsigned long other_imps = text_ads_imps - imps;

          if(other_imps != 0)
          {
            const RevenueDecimal other_revenue = (
              ch_cnt_it != text_imp_channels.end() ?
              sum_request_revenue - ch_cnt_it->second.revenue :
              sum_request_revenue);

            inv_info.text_imps.imp_other_channels.emplace_back(
              *ch_it,
              InventoryActionProcessor::InventoryInfo::ChannelImpCounter(
                other_imps, other_revenue));
          }

          unsigned long impops = request_info.max_text_ads - text_ads_imps;

          if(impops != 0)
          {
            try
            {
              // impop revenue = <cost threshold> / <max text ads> * <impops>
              RevenueDecimal impop_revenue_div_reminder;
              RevenueDecimal impop_revenue = RevenueDecimal::div(
                RevenueDecimal::mul(
                  request_info.cost_threshold,
                  RevenueDecimal(false, impops, 0),
                  Generics::DMR_FLOOR),
                RevenueDecimal(false, request_info.max_text_ads, 0),
                impop_revenue_div_reminder);
              impop_revenue += impop_revenue_div_reminder;

              inv_info.text_imps.impop_no_imp_channels.emplace_back(
                *ch_it,
                InventoryActionProcessor::InventoryInfo::ChannelImpCounter(
                  impops, impop_revenue));
            }
            catch(...)
            {
              assert(0);
            }
          }
        }
      }
      else
      {
        inv_info.display_imps.impop_no_imp_channels.reserve(triggered_expression_channels.size());
        inv_info.text_imps.impop_no_imp_channels.reserve(triggered_expression_channels.size());

        // impression opportunity, but display or text ads isn't selected
        for(auto ch_it = triggered_expression_channels.begin();
            ch_it != triggered_expression_channels.end(); ++ch_it)
        {
          inv_info.display_imps.impop_no_imp_channels.emplace_back(
            *ch_it,
            InventoryActionProcessor::InventoryInfo::ChannelImpCounter(
              1, request_info.cost_threshold));

          if(request_info.max_text_ads != 0)
          {
            inv_info.text_imps.impop_no_imp_channels.emplace_back(
              *ch_it,
              InventoryActionProcessor::InventoryInfo::ChannelImpCounter(
                request_info.max_text_ads, request_info.cost_threshold));
            // text_cost_threshold ?
          }
        }
      }
    }

    return !inv_info.display_imps.empty() || !inv_info.text_imps.empty();
  }
} /* namespace RequestInfoSvcs */
} /* namespace AdServer */

