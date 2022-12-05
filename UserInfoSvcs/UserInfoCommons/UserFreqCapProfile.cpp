#include <sstream>
#include <vector>
#include <Generics/Time.hpp>

#include <Commons/Algs.hpp>
#include <UtilCommons/Table.hpp>

#include "UserFreqCapProfile.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
namespace
{
  const Table::Column FC_TABLE_COLUMNS[] =
  {
    Table::Column("fc_id", Table::Column::NUMBER),
    Table::Column("virtual", Table::Column::NUMBER),
    Table::Column("total_impressions", Table::Column::NUMBER),
    Table::Column("last_impressions", Table::Column::TEXT)
  };

  const Table::Column UCFC_TABLE_COLUMNS[] =
  {
    Table::Column("request_id", Table::Column::TEXT),
    Table::Column("time", Table::Column::TEXT),
    Table::Column("fc_ids", Table::Column::TEXT),
    Table::Column("imps", Table::Column::TEXT)
  };

  const Table::Column SEQ_ORDER_TABLE_COLUMNS[] =
  {
    Table::Column("ccg_id", Table::Column::NUMBER),
    Table::Column("set_id", Table::Column::NUMBER),
    Table::Column("imps", Table::Column::NUMBER),
  };

  const Table::Column PUBLISHER_ACCOUNTS_TABLE_COLUMNS[] =
  {
    Table::Column("publisher_account_id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column CAMPAIGN_FREQ_TABLE_COLUMNS[] =
  {
    Table::Column("campaign_id", Table::Column::NUMBER),
    Table::Column("imps", Table::Column::TEXT),
  };

  const AdServer::Commons::FreqCap DEFAULT_FREQ_CAP(0, 1, Generics::Time::ZERO, 100, Generics::Time(365 * 24 * 3600));

  const AdServer::Commons::FreqCap& find_or_get_default(
    const AdServer::UserInfoSvcs::FreqCapConfig::FreqCapMap& freq_caps,
    uint32_t fc_id)
  {
    AdServer::UserInfoSvcs::FreqCapConfig::FreqCapMap::const_iterator fc_it =
        freq_caps.find(fc_id);

    return (fc_it != freq_caps.end()) ? fc_it->second : DEFAULT_FREQ_CAP;
  }

  const uint32_t IMP_TIMESTAMP_MASK = 0x7FFFFFFF;

  inline
  uint32_t
  normalize_impression_timestamp(uint32_t value)
  {
    return (value & IMP_TIMESTAMP_MASK);
  }

  inline
  uint32_t
  is_uc_timestamp(uint32_t value)
  {
    return (value & ~IMP_TIMESTAMP_MASK);
  }

  inline
  uint32_t
  to_uc_timestamp(uint32_t value)
  {
    return (value | ~IMP_TIMESTAMP_MASK);
  }

  struct FreqCapCounter
  {
    FreqCapCounter(
      unsigned long fc_id_val,
      unsigned long count_val,
      unsigned long uc_count_val)
      : fc_id(fc_id_val),
        count(count_val),
        uc_count(uc_count_val)
    {}

    unsigned long fc_id;
    unsigned long count;
    unsigned long uc_count;
  };

  inline
  void
  add_uc_fc_counter(
    FreqCapWriter::last_impressions_Container& last_impressions,
    unsigned long uc_count,
    const Generics::Time& now)
  {
    for(unsigned long i = 0; i < uc_count; ++i)
    {
      last_impressions.push_back(to_uc_timestamp(now.tv_sec));
    }
  };

  inline
  void
  add_fc_counter(
    FreqCapWriter::last_impressions_Container& last_impressions,
    unsigned long count,
    const Generics::Time& now)
  {
    for(unsigned long i = 0; i < count; ++i)
    {
      last_impressions.push_back(now.tv_sec);
    }
  };

  struct LastImpLess
  {
    bool
    operator()(uint32_t left, uint32_t right) const
    {
      uint32_t left_norm = normalize_impression_timestamp(left);
      uint32_t right_norm = normalize_impression_timestamp(right);

      if(left_norm < right_norm)
      {
        return true;
      }

      if(left_norm == right_norm)
      {
        // for equal timestamp's uc less then non uc
        return is_uc_timestamp(right) < is_uc_timestamp(left);
      }

      return false;
    }
  };

  struct FreqCapCounterLess
  {
    bool
    operator()(const FreqCapCounter& left, const FreqCapCounter& right) const
    {
      return left.fc_id < right.fc_id;
    }
  };

  struct FreqCapCounterMerger
  {
    FreqCapCounter operator()(
      const FreqCapCounter& left,
      const FreqCapCounter& right)
      const
    {
      return FreqCapCounter(
        left.fc_id,
        left.count + right.count,
        left.uc_count + right.uc_count);
    }
  };

  typedef std::vector<FreqCapCounter> FreqCapCounterVector;

  struct FreqCapLess
  {
    bool operator()(const FreqCapReader& left, const FreqCapReader& right) const
      noexcept
    {
      return left.fc_id() < right.fc_id();
    }

    bool operator()(const FreqCapWriter& left, const FreqCapCounter& right) const
      noexcept
    {
      return left.fc_id() < right.fc_id;
    }

    bool operator()(const FreqCapCounter& left, const FreqCapWriter& right) const
      noexcept
    {
      return left.fc_id < right.fc_id();
    }
  };

  struct UcFreqCapLess
  {
    bool operator()(const UcFreqCapReader& left, const UcFreqCapReader& right) const
      noexcept
    {
      return left.time() < right.time() ||
        (left.time() == right.time() && left.request_id() < right.request_id());
    }
  };

  struct FreqCapFiller
  {
    const FreqCapWriter&
    operator()(const FreqCapWriter& val) const
      noexcept
    {
      return val;
    }

    FreqCapWriter
    operator()(const FreqCapReader& val) const
      noexcept
    {
      FreqCapWriter res;
      res.fc_id() = val.fc_id();
      res.total_impressions() = val.total_impressions();
      std::copy(val.last_impressions().begin(), val.last_impressions().end(),
        std::back_inserter(res.last_impressions()));
      return res;
    }
  };

  class FreqCapAdapter
  {
    // convert fc_id impression to FreqCap record and
    //   clear expired impressions
  public:
    FreqCapAdapter(const Generics::Time& now,
      const FreqCapConfig& fc_config) noexcept
      : now_(now),
        fc_config_(fc_config)
    {}

    FreqCapWriter
    operator()(const FreqCapWriter& val) const
      noexcept
    {
      const Commons::FreqCap& freq_cap = find_or_get_default(fc_config_.freq_caps, val.fc_id());

      FreqCapWriter res;
      res.fc_id() = val.fc_id();
      res.total_impressions() = val.total_impressions();

      const FreqCapWriter::last_impressions_Container& last_imps =
        val.last_impressions();

      if(!last_imps.empty())
      {
        res.last_impressions().reserve(last_imps.size());

        unsigned long imp_count = 0;
        FreqCapWriter::last_impressions_Container::const_reverse_iterator
          clear_end = last_imps.rbegin();

        if(freq_cap.period != Generics::Time::ZERO &&
            freq_cap.period + normalize_impression_timestamp(*clear_end) > now_)
        {
          // don't count unconfirmed timestamp's (see ADSC-9494)
          imp_count = is_uc_timestamp(*clear_end) ? 0 : 1;
          ++clear_end;
        }

        if(freq_cap.window_limit)
        {
          unsigned long bound_time = freq_cap.window_time < now_ ?
            (now_ - freq_cap.window_time).tv_sec : 0;
          unsigned long window_count = imp_count;

          while(clear_end != last_imps.rend() &&
            window_count <= freq_cap.window_limit &&
            normalize_impression_timestamp(*clear_end) >= bound_time)
          {
            // don't count unconfirmed timestamp's (see ADSC-9494)
            if(!is_uc_timestamp(*clear_end))
            {
              ++window_count;
            }
            ++clear_end;
          }
        }

        std::copy(clear_end.base(), last_imps.end(),
          std::back_inserter(res.last_impressions()));
      }

      return res;
    }

    FreqCapWriter
    operator()(const FreqCapCounter& fc_counter) noexcept
    {
      const Commons::FreqCap& freq_cap = find_or_get_default(fc_config_.freq_caps, fc_counter.fc_id);

      FreqCapWriter res;
      res.fc_id() = fc_counter.fc_id;
      res.total_impressions() = fc_counter.count + fc_counter.uc_count;
      if(freq_cap.window_limit ||
        freq_cap.period != Generics::Time::ZERO)
      {
        add_fc_counter(res.last_impressions(), fc_counter.count, now_);
        add_uc_fc_counter(res.last_impressions(), fc_counter.uc_count, now_);
      }

      return res;
    }

  private:
    Generics::Time now_;
    const FreqCapConfig& fc_config_;
  };

  class UcFreqCapAdapter
  {
  public:
    UcFreqCapAdapter() noexcept
    {}

    const UcFreqCapWriter&
    operator()(const UcFreqCapWriter& val) const
      noexcept
    {
      return val;
    }

    UcFreqCapWriter
    operator()(const UcFreqCapReader& reader) const
      noexcept
    {
      UcFreqCapWriter writer;
      writer.request_id() = reader.request_id();
      writer.time() = reader.time();
      std::copy(reader.freq_cap_ids().begin(), reader.freq_cap_ids().end(),
        std::back_inserter(writer.freq_cap_ids()));
      std::copy(reader.imps().begin(), reader.imps().end(),
        std::back_inserter(writer.imps()));
      return writer;
    }
  };

  class FreqCapMerger
  {
  public:
    FreqCapMerger(const Generics::Time& now) noexcept
      : now_(now)
    {}

    FreqCapWriter operator()(
      const FreqCapWriter& fc_writer,
      const FreqCapCounter& fc_counter)
    {
      FreqCapWriter res;
      res.fc_id() = fc_counter.fc_id;
      res.total_impressions() = fc_writer.total_impressions() +
        fc_counter.count + fc_counter.uc_count;

      bool uc_inserted = false;
      bool inserted = false;

      for(FreqCapWriter::last_impressions_Container::const_iterator fc_imp_it =
            fc_writer.last_impressions().begin();
          fc_imp_it != fc_writer.last_impressions().end(); ++fc_imp_it)
      {
        if(!inserted && *fc_imp_it > now_.tv_sec)
        {
          add_fc_counter(res.last_impressions(), fc_counter.count, now_);
          inserted = true;
        }

        if(!uc_inserted && normalize_impression_timestamp(*fc_imp_it) > now_.tv_sec)
        {
          add_uc_fc_counter(res.last_impressions(), fc_counter.uc_count, now_);
          uc_inserted = true;
        }

        res.last_impressions().push_back(*fc_imp_it);
      }

      if(!inserted)
      {
        add_fc_counter(res.last_impressions(), fc_counter.count, now_);
      }
      
      if(!uc_inserted)
      {
        add_uc_fc_counter(res.last_impressions(), fc_counter.uc_count, now_);
      }

      return res;
    }

    FreqCapWriter operator()(
      const FreqCapReader& left,
      const FreqCapReader& right)
    {
      FreqCapWriter res;
      res.fc_id() = left.fc_id();
      res.total_impressions() = left.total_impressions() + right.total_impressions();
      std::merge(left.last_impressions().begin(),
        left.last_impressions().end(),
        right.last_impressions().begin(),
        right.last_impressions().end(),
        std::back_inserter(res.last_impressions()),
        LastImpLess());

      return res;
    }

  private:
    Generics::Time now_;
  };

  struct FreqCapFilter
  {
    bool operator()(const FreqCapWriter& val) const noexcept
    {
      return val.fc_id() != 0;
    }
  };

  struct FreqCapDecrease
  {
    bool operator<(const FreqCapDecrease& right) const noexcept
    {
      return fc_id < right.fc_id || (fc_id == right.fc_id &&
        time < right.time);
    }

    unsigned long fc_id;
    uint32_t time;
  };

  struct FreqCapDecreaseCounter
  {
    FreqCapDecreaseCounter()
      : count(0)
    {}

    unsigned long count;
  };

  struct SeqOrderAdapter
  {
    const SeqOrderWriter&
    operator()(const SeqOrderWriter& val) const
      noexcept
    {
      return val;
    }

    SeqOrderWriter
    operator()(const UserFreqCapProfile::SeqOrder& seq_order) noexcept
    {
      SeqOrderWriter writer;
      writer.ccg_id() = seq_order.ccg_id;
      writer.set_id() = seq_order.set_id;
      writer.imps() = seq_order.imps;
      return writer;
    }
  };

  struct SeqOrderLess
  {
    bool operator()(
      const SeqOrderReader& left,
      const SeqOrderReader& right) const
      noexcept
    {
      return left.ccg_id() < right.ccg_id();
    }

    bool operator()(
      const SeqOrderWriter& left,
      const UserFreqCapProfile::SeqOrder& right) const
      noexcept
    {
      return left.ccg_id() < right.ccg_id;
    }

    bool operator()(
      const UserFreqCapProfile::SeqOrder& left,
      const SeqOrderWriter& right) const
      noexcept
    {
      return left.ccg_id < right.ccg_id();
    }
  };

  struct SeqOrderMerger
  {
    SeqOrderWriter operator()(
      const SeqOrderWriter& writer,
      const UserFreqCapProfile::SeqOrder& seq_order)
    {
      SeqOrderWriter res;
      res.ccg_id() = seq_order.ccg_id;
      res.set_id() = seq_order.set_id;
      res.imps() = (
        writer.set_id() == seq_order.set_id ?
        writer.imps() + seq_order.imps :
        seq_order.imps);
      
      return res;
    }

    SeqOrderWriter operator()(
      const SeqOrderReader& left,
      const SeqOrderReader& right)
    {
      SeqOrderWriter res;
      res.ccg_id() = left.ccg_id();
      res.set_id() = left.set_id();
      res.imps() = (
        left.set_id() == right.set_id() ?
        left.imps() + right.imps() :
        right.imps());
      
      return res;
    }
  };

  struct PublisherAccountLess
  {
    template<typename LeftType, typename RightType>
    bool operator()(
      const LeftType& left,
      const RightType& right) const
      noexcept
    {
      return (left.publisher_account_id() < right.publisher_account_id());
    }
  };

  struct PublisherAccountMerger
  {
    template<typename LeftType, typename RightType>
    PublisherAccountWriter operator()(
      const LeftType& left,
      const RightType& right)
    {
      PublisherAccountWriter res;
      res.publisher_account_id() = left.publisher_account_id();
      res.timestamp() = std::max(left.timestamp(), right.timestamp());

      return res;
    }
  };

  struct CampaignFreqLess
  {
    bool operator()(
      const CampaignFreqWriter& left,
      unsigned int campaign_id) const
      noexcept
    {
      return left.campaign_id() < campaign_id;
    }

    bool operator()(
      const CampaignFreqReader& left,
      const CampaignFreqReader& right) const
      noexcept
    {
      return left.campaign_id() < right.campaign_id();
    }
  };

  struct CampaignFreqMerger
  {
    CampaignFreqWriter operator()(
      const CampaignFreqReader& left,
      const CampaignFreqReader& right)
    {
      CampaignFreqWriter res;
      res.campaign_id() = left.campaign_id();
      res.imps() = left.imps() + right.imps();
      return res;
    }
  };

  void check_full_freq_caps(
    UserFreqCapProfile::FreqCapIdList& fcs,
    const Generics::Time& now,
    const FreqCapConfig& fc_config,
    const UserFreqCapProfileReader::freq_caps_Container& freq_caps)
  {
    for(UserFreqCapProfileReader::freq_caps_Container::
          const_iterator fc_it = freq_caps.begin();
        fc_it != freq_caps.end(); ++fc_it)
    {
      const Commons::FreqCap& freq_cap = find_or_get_default(fc_config.freq_caps, (*fc_it).fc_id());

      // check freq cap
      if(freq_cap.lifelimit &&
         freq_cap.lifelimit <= (*fc_it).total_impressions())
      {
        fcs.push_back((*fc_it).fc_id());
      }
      else if(!(*fc_it).last_impressions().empty())
      {
        if(freq_cap.period != Generics::Time::ZERO &&
            Generics::Time(
              normalize_impression_timestamp(
                *--(*fc_it).last_impressions().end())) +
              freq_cap.period > now)
        {
          fcs.push_back((*fc_it).fc_id());
        }
        else if(freq_cap.window_limit)
        {
          unsigned long window_bound = now > freq_cap.window_time ?
            (now - freq_cap.window_time).tv_sec : 0;

          FreqCapReader::last_impressions_Container::const_iterator imp_it_end =
            (*fc_it).last_impressions().end();

          FreqCapReader::last_impressions_Container::const_iterator imp_it =
            (*fc_it).last_impressions().begin();
          while(imp_it != imp_it_end &&
             normalize_impression_timestamp(*imp_it) < window_bound)
          {
            ++imp_it;
          }

          if(static_cast<unsigned long>(imp_it_end - imp_it) >=
              freq_cap.window_limit)
          {
            fcs.push_back((*fc_it).fc_id());
          }
        }
      }
    }
  }

  void
  global_check_freq_caps(
    UserFreqCapProfile::FreqCapIdList& fcs,
    UserFreqCapProfile::FreqCapIdList* virtual_fcs,
    const Generics::Time& now,
    const FreqCapConfig& fc_config,
    const UserFreqCapProfileReader& reader)
  {
    check_full_freq_caps(
      fcs,
      now,
      fc_config,
      reader.freq_caps());

    if(virtual_fcs)
    {
      check_full_freq_caps(
        *virtual_fcs,
        now,
        fc_config,
        reader.virtual_freq_caps());
    }
  }

  void
  read_campaign_freqs(
    UserFreqCapProfile::CampaignFreqs& campaign_freqs,
    const UserFreqCapProfileReader& reader) noexcept
  {
    const auto reader_campaign_freqs = reader.campaign_freqs();

    for (auto it = reader_campaign_freqs.begin();
         it != reader_campaign_freqs.end(); ++it)
    {
      const CampaignFreqReader& campaign_freq = *it;
      campaign_freqs.emplace_back(UserFreqCapProfile::CampaignFreq
        {campaign_freq.campaign_id(), campaign_freq.imps()});
    }
  }

  void
  clean_up_campaign_freqs_(
    UserFreqCapProfileWriter& profile,
    const FreqCapConfig::CampaignIds& campaign_ids)
    /*throw(eh::Exception)*/
  {
    for (auto it = profile.campaign_freqs().begin();
         it != profile.campaign_freqs().end();)
    {
      if (campaign_ids.find((*it).campaign_id()) == campaign_ids.end())
      {
        it = profile.campaign_freqs().erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  void
  clean_up(
    UserFreqCapProfileWriter& profile,
    const Generics::Time& low_bound) noexcept
  {
    typedef std::map<FreqCapDecrease, FreqCapDecreaseCounter> FreqCapDecreaseSet;

    FreqCapDecreaseSet fc_dec_set;

    UserFreqCapProfileWriter::uc_freq_caps_Container&
      uc_freq_caps = profile.uc_freq_caps();

    UserFreqCapProfileWriter::uc_freq_caps_Container::
      iterator clear_end = uc_freq_caps.begin();

    while(clear_end != uc_freq_caps.end() &&
          low_bound.tv_sec > clear_end->time())
    {
      const UcFreqCapWriter::freq_cap_ids_Container& fcs =
        clear_end->freq_cap_ids();

      for(UcFreqCapWriter::freq_cap_ids_Container::const_iterator fc_it =
            fcs.begin();
          fc_it != fcs.end(); ++fc_it)
      {
        FreqCapDecrease fc_dec;
        fc_dec.fc_id = *fc_it;
        fc_dec.time = clear_end->time();
        ++fc_dec_set[fc_dec].count;
      }

      ++clear_end;
    }

    uc_freq_caps.erase(uc_freq_caps.begin(), clear_end);

    // clear counters from freq caps section
    UserFreqCapProfileWriter::freq_caps_Container::iterator fc_it =
        profile.freq_caps().begin();
    FreqCapDecreaseSet::const_iterator fc_dec_it = fc_dec_set.begin();

    while(fc_it != profile.freq_caps().end() && fc_dec_it != fc_dec_set.end())
    {
      if(fc_it->fc_id() < fc_dec_it->first.fc_id)
      {
        ++fc_it;
      }
      else
      {
        if(fc_it->fc_id() == fc_dec_it->first.fc_id)
        {
          uint32_t uc_time = to_uc_timestamp(fc_dec_it->first.time);

          FreqCapWriter::last_impressions_Container& imps = fc_it->last_impressions();
          FreqCapWriter::last_impressions_Container::iterator imp_it = std::lower_bound(
            imps.begin(),
            imps.end(),
            uc_time,
            LastImpLess());

          unsigned long imp_i = 0;
          if(imp_it != imps.end() && *imp_it == uc_time)
          {
            FreqCapWriter::last_impressions_Container::iterator imp_erase_end_it = imp_it;
            do
            {
              ++imp_i;
              ++imp_erase_end_it;
            }
            while(imp_erase_end_it != imps.end() &&
              *imp_erase_end_it == uc_time && imp_i < fc_dec_it->second.count);

            imps.erase(imp_it, imp_erase_end_it);
          }

          if(fc_it->total_impressions() >= fc_dec_it->second.count)
          {
            fc_it->total_impressions() -= fc_dec_it->second.count;
          }
          else
          {
            fc_it->total_impressions() = 0;
          }

          if(imps.empty() && fc_it->total_impressions() == 0)
          {
            profile.freq_caps().erase(fc_it++);
          }
        }

        ++fc_dec_it;
      }
    }
  }

  void
  pack_freq_caps(
    FreqCapCounterVector& fcs_res,
    const UserFreqCapProfile::FreqCapIdList& fcs,
    bool unconfirmed)
    noexcept
  {
    fcs_res.reserve(fcs_res.size() + fcs.size());
    for(UserFreqCapProfile::FreqCapIdList::const_iterator fc_it = fcs.begin();
        fc_it != fcs.end(); ++fc_it)
    {
      if(fcs_res.empty() || fcs_res.back().fc_id != *fc_it)
      {
        fcs_res.push_back(FreqCapCounter(
          *fc_it, unconfirmed ? 0 : 1, unconfirmed ? 1 : 0));
      }
      else
      {
        ++fcs_res.back().count;
      }
    }
  }

  template<typename Cont>
  void
  update_campaign_freq(
    const Cont& imps,
    UserFreqCapProfileWriter& writer)
    /*throw(eh::Exception)*/
  {
    for (auto it = imps.begin(); it != imps.end(); ++it)
    {
      const auto campaign_freqs_it = std::lower_bound(
        writer.campaign_freqs().begin(),
        writer.campaign_freqs().end(),
        *it,
        CampaignFreqLess());

      if (campaign_freqs_it != writer.campaign_freqs().end() &&
          campaign_freqs_it->campaign_id() == *it)
      {
        campaign_freqs_it->imps() += 1;
      }
      else
      {
        CampaignFreqWriter campaign_freq_writer;
        campaign_freq_writer.campaign_id() = *it;
        campaign_freq_writer.imps() = 1;
        writer.campaign_freqs().insert(campaign_freqs_it, campaign_freq_writer);
      }
    }
  }
} // namespace

  UserFreqCapProfile::UserFreqCapProfile(ConstSmartMemBufPtr plain_profile)
    /*throw(Invalid)*/
    : plain_profile_(new SmartMemBuf())
  {
    static const char* FUN = "UserFreqCapProfile::UserFreqCapProfile()";

    if(plain_profile)
    {
      if(plain_profile->membuf().size() > 40*1024*1024)
      {
        Stream::Error ostr;
        ostr << FUN << ": incorrect profile size: " << plain_profile->membuf().size();
        throw Invalid(ostr);
      }

      plain_profile_->membuf().assign(
        plain_profile->membuf().data(),
        plain_profile->membuf().size());
    }
  }

  bool
  UserFreqCapProfile::full(
    FreqCapIdList& fcs,
    FreqCapIdList* virtual_fcs,
    SeqOrderList& seq_orders,
    CampaignFreqs& campaign_freqs,
    const Generics::Time& now,
    const FreqCapConfig& fc_config)
    /*throw(eh::Exception)*/
  {
    if(!plain_profile_->membuf().size())
    {
      return false;
    }

    UserFreqCapProfileReader reader(
      plain_profile_->membuf().data(),
      plain_profile_->membuf().size());

    for(UserFreqCapProfileReader::seq_orders_Container::
          const_iterator it = reader.seq_orders().begin();
        it != reader.seq_orders().end(); ++it)
    {
      SeqOrder seq_order;
      seq_order.ccg_id = (*it).ccg_id();
      seq_order.set_id = (*it).set_id();
      seq_order.imps = (*it).imps();
      seq_orders.push_back(seq_order);
    }

    if(!reader.uc_freq_caps().empty() &&
       fc_config.confirm_timeout + (*reader.uc_freq_caps().begin()).time() <
         now)
    {
      UserFreqCapProfileWriter writer(
        plain_profile_->membuf().data(),
        plain_profile_->membuf().size());

      clean_up(writer, now - fc_config.confirm_timeout);
      clean_up_campaign_freqs_(writer, fc_config.campaign_ids);

      Generics::MemBuf mb(writer.size());
      plain_profile_->membuf().assign(mb.data(), mb.size());
      writer.save(
        plain_profile_->membuf().data(), plain_profile_->membuf().size());

      UserFreqCapProfileReader new_reader(
        plain_profile_->membuf().data(),
        plain_profile_->membuf().size());

      global_check_freq_caps(
        fcs,
        virtual_fcs,
        now,
        fc_config,
        new_reader);

      read_campaign_freqs(campaign_freqs, new_reader);
    }
    else
    {
      global_check_freq_caps(
        fcs,
        virtual_fcs,
        now,
        fc_config,
        reader);

      read_campaign_freqs(campaign_freqs, reader);
    }

    return true;
  }

  void
  UserFreqCapProfile::consider(
    const Commons::RequestId& request_id,
    const Generics::Time& now,
    const FreqCapIdList& c_fcs,
    const FreqCapIdList& uc_fcs,
    const FreqCapIdList& virtual_fcs,
    const SeqOrderList& seq_orders,
    const CampaignIds& campaign_ids,
    const CampaignIds& uc_campaign_ids,
    const FreqCapConfig& fc_config) noexcept
  {
    UserFreqCapProfileWriter writer;

    if(plain_profile_->membuf().size())
    {
      writer.init(
        plain_profile_->membuf().data(),
        plain_profile_->membuf().size());
    }
    else
    {
      writer.version() = CURRENT_FREQ_CAP_PROFILE_VERSION;
    }

    UserFreqCapProfileWriter::freq_caps_Container new_freq_caps;
    FreqCapCounterVector packed_fcs;

    {
      FreqCapCounterVector packed_c_fcs;
      pack_freq_caps(packed_c_fcs, c_fcs, false);

      FreqCapCounterVector packed_uc_fcs;
      pack_freq_caps(packed_uc_fcs, uc_fcs, true);

      Algs::merge_unique(
        packed_c_fcs.begin(),
        packed_c_fcs.end(),
        packed_uc_fcs.begin(),
        packed_uc_fcs.end(),
        std::back_inserter(packed_fcs),
        FreqCapCounterLess(),
        FreqCapCounterMerger());
    }

    Algs::merge_unique(
      writer.freq_caps().begin(),
      writer.freq_caps().end(),
      packed_fcs.begin(),
      packed_fcs.end(),
      Algs::modify_inserter(
        Algs::filter_inserter(std::back_inserter(new_freq_caps), FreqCapFilter()),
        FreqCapAdapter(now, fc_config)),
      FreqCapLess(),
      FreqCapMerger(now));

    writer.freq_caps().swap(new_freq_caps);

    UserFreqCapProfileWriter::freq_caps_Container new_virtual_freq_caps;
    FreqCapCounterVector packed_virtual_fcs;
    pack_freq_caps(packed_virtual_fcs, virtual_fcs, false);

    Algs::merge_unique(
      writer.virtual_freq_caps().begin(),
      writer.virtual_freq_caps().end(),
      packed_virtual_fcs.begin(),
      packed_virtual_fcs.end(),
      Algs::modify_inserter(
        Algs::filter_inserter(std::back_inserter(new_virtual_freq_caps), FreqCapFilter()),
        FreqCapAdapter(now, fc_config)),
      FreqCapLess(),
      FreqCapMerger(now));

    writer.virtual_freq_caps().swap(new_virtual_freq_caps);

    // save markers for unconfirmed freq cap's
    if(!request_id.is_null() && (!uc_fcs.empty() || !uc_campaign_ids.empty()))
    {
      UserFreqCapProfileWriter::uc_freq_caps_Container::
        reverse_iterator uc_it = writer.uc_freq_caps().rbegin();

      while(uc_it != writer.uc_freq_caps().rend() &&
        now.tv_sec < uc_it->time())
      {
        ++uc_it;
      }

      UcFreqCapWriter new_uc_fc_writer;
      new_uc_fc_writer.request_id() = request_id.to_string();
      new_uc_fc_writer.time() = now.tv_sec;
      std::copy(uc_fcs.begin(), uc_fcs.end(),
        std::back_inserter(new_uc_fc_writer.freq_cap_ids()));
      new_uc_fc_writer.imps().insert(
        new_uc_fc_writer.imps().end(), uc_campaign_ids.begin(), uc_campaign_ids.end());
      writer.uc_freq_caps().insert(uc_it.base(), new_uc_fc_writer);
    }

    // update seq orders
    UserFreqCapProfileWriter::seq_orders_Container new_seq_orders;

    Algs::merge_unique(
      writer.seq_orders().begin(),
      writer.seq_orders().end(),
      seq_orders.begin(),
      seq_orders.end(),
      Algs::modify_inserter(
        std::back_inserter(new_seq_orders), SeqOrderAdapter()),
      SeqOrderLess(),
      SeqOrderMerger());

    update_campaign_freq(campaign_ids, writer);
    clean_up_campaign_freqs_(writer, fc_config.campaign_ids);

    writer.seq_orders().swap(new_seq_orders);

    if(!writer.uc_freq_caps().empty() &&
       fc_config.confirm_timeout + (*writer.uc_freq_caps().begin()).time() <
         now)
    {
      clean_up(writer, now - fc_config.confirm_timeout);
    }

    Generics::MemBuf mb(writer.size());
    plain_profile_->membuf().assign(mb.data(), mb.size());
    writer.save(
      plain_profile_->membuf().data(), plain_profile_->membuf().size());
  }

  bool
  UserFreqCapProfile::confirm_request(
    const Commons::RequestId& request_id,
    const Generics::Time& now,
    const FreqCapConfig& fc_config)
    noexcept
  {
    if(!plain_profile_->membuf().size())
    {
      return false;
    }

    UserFreqCapProfileWriter writer(
      plain_profile_->membuf().data(),
      plain_profile_->membuf().size());

    const std::string req_id = request_id.to_string();
    UserFreqCapProfileWriter::uc_freq_caps_Container::iterator
      uc_fc_it = writer.uc_freq_caps().begin();

    bool modified = false;

    while(uc_fc_it != writer.uc_freq_caps().end())
    {
      if(req_id == uc_fc_it->request_id())
      {
        FreqCapAdapter fc_adapter(now, fc_config);

        // convert unconfirmed timestamp's to confirmed
        UserFreqCapProfileWriter::freq_caps_Container::iterator fc_it =
          writer.freq_caps().begin();

        UcFreqCapWriter::freq_cap_ids_Container::const_iterator uc_fc_id_it =
          uc_fc_it->freq_cap_ids().begin();

        while(uc_fc_id_it != uc_fc_it->freq_cap_ids().end() &&
          fc_it != writer.freq_caps().end())
        {
          if(*uc_fc_id_it == fc_it->fc_id())
          {
            uint32_t uc_time = to_uc_timestamp(uc_fc_it->time());

            FreqCapWriter::last_impressions_Container::iterator imp_it = std::lower_bound(
              fc_it->last_impressions().begin(),
              fc_it->last_impressions().end(),
              uc_time,
              LastImpLess());

            if(imp_it != fc_it->last_impressions().end() && *imp_it == uc_time)
            {
              // uc less then non uc, see LastImpLess
              // search end of equal uc timestamp's
              imp_it = fc_it->last_impressions().erase(imp_it);
              while(imp_it != fc_it->last_impressions().end() && *imp_it == uc_time)
              {
                ++imp_it;
              }
              fc_it->last_impressions().insert(imp_it, uc_fc_it->time());
            }

            *fc_it = fc_adapter(*fc_it); // clean up extended cell

            ++uc_fc_id_it;
            ++fc_it;
          }
          else if(*uc_fc_id_it < fc_it->fc_id())
          {
            ++uc_fc_id_it;
          }
          else
          {
            ++fc_it;
          }
        }

        // modify campaign_freqs
        update_campaign_freq(uc_fc_it->imps(), writer);

        writer.uc_freq_caps().erase(uc_fc_it);

        modified = true;
        break;
      }

      ++uc_fc_it;
    }

    if(!writer.uc_freq_caps().empty() &&
       fc_config.confirm_timeout + (*writer.uc_freq_caps().begin()).time() <
         now)
    {
      clean_up(writer, now - fc_config.confirm_timeout);
      modified = true;
    }

    if(modified)
    {
      clean_up_campaign_freqs_(writer, fc_config.campaign_ids);

      Generics::MemBuf mb(writer.size());
      plain_profile_->membuf().assign(mb.data(), mb.size());
      writer.save(plain_profile_->membuf().data(), plain_profile_->membuf().size());
    }

    return modified;
  }

  void
  UserFreqCapProfile::merge(
    ConstSmartMemBufPtr merge_profile,
    const Generics::Time& now,
    const FreqCapConfig& fc_config)
    /*throw(eh::Exception)*/
  {
    if(!merge_profile->membuf().empty())
    {
      if(!plain_profile_->membuf().empty())
      {
        UserFreqCapProfileReader reader(
          plain_profile_->membuf().data(),
          plain_profile_->membuf().size());

        UserFreqCapProfileReader merge_reader(
          merge_profile->membuf().data(),
          merge_profile->membuf().size());

        UserFreqCapProfileWriter result_writer;

        result_writer.version() = CURRENT_FREQ_CAP_PROFILE_VERSION;

        Algs::merge_unique(
          reader.freq_caps().begin(),
          reader.freq_caps().end(),
          merge_reader.freq_caps().begin(),
          merge_reader.freq_caps().end(),
          Algs::modify_inserter(
            Algs::modify_inserter(
              std::back_inserter(result_writer.freq_caps()),
              FreqCapFiller()),
            FreqCapAdapter(now, fc_config)),
          FreqCapLess(),
          FreqCapMerger(now));

        std::merge(
          reader.uc_freq_caps().begin(),
          reader.uc_freq_caps().end(),
          merge_reader.uc_freq_caps().begin(),
          merge_reader.uc_freq_caps().end(),
          Algs::modify_inserter(
            std::back_inserter(result_writer.uc_freq_caps()),
            UcFreqCapAdapter()),
          UcFreqCapLess());

        Algs::merge_unique(
          reader.virtual_freq_caps().begin(),
          reader.virtual_freq_caps().end(),
          merge_reader.virtual_freq_caps().begin(),
          merge_reader.virtual_freq_caps().end(),
          Algs::modify_inserter(
            std::back_inserter(result_writer.virtual_freq_caps()),
            FreqCapAdapter(now, fc_config)),
          FreqCapLess(),
          FreqCapMerger(now));

        Algs::merge_unique(
          reader.seq_orders().begin(),
          reader.seq_orders().end(),
          merge_reader.seq_orders().begin(),
          merge_reader.seq_orders().end(),
          std::back_inserter(result_writer.seq_orders()),
          SeqOrderLess(),
          SeqOrderMerger());

        Algs::merge_unique(
          reader.publisher_accounts().begin(),
          reader.publisher_accounts().end(),
          merge_reader.publisher_accounts().begin(),
          merge_reader.publisher_accounts().end(),
          std::back_inserter(result_writer.publisher_accounts()),
          PublisherAccountLess(),
          PublisherAccountMerger());

        Algs::merge_unique(
          reader.campaign_freqs().begin(),
          reader.campaign_freqs().end(),
          merge_reader.campaign_freqs().begin(),
          merge_reader.campaign_freqs().end(),
          std::back_inserter(result_writer.campaign_freqs()),
          CampaignFreqLess(),
          CampaignFreqMerger());
        clean_up_campaign_freqs_(result_writer, fc_config.campaign_ids);

        if(!result_writer.uc_freq_caps().empty() &&
           fc_config.confirm_timeout + (*result_writer.uc_freq_caps().begin()).time() <
             now)
        {
          clean_up(result_writer, now - fc_config.confirm_timeout);
        }

        Generics::MemBuf mb(result_writer.size());
        plain_profile_->membuf().assign(mb.data(), mb.size());
        result_writer.save(
          plain_profile_->membuf().data(), plain_profile_->membuf().size());
      }
      else
      {
        plain_profile_ =
          Algs::copy_membuf(merge_profile);
      }
    }
  }

  bool
  UserFreqCapProfile::consider_publishers_optin(
    const std::set<unsigned long>& publisher_account_ids,
    const Generics::Time& timestamp) noexcept
  {
    if (publisher_account_ids.empty())
    {
      return false;
    }
    
    UserFreqCapProfileWriter writer;

    if(plain_profile_->membuf().size() != 0)
    {
      writer.init(
        plain_profile_->membuf().data(),
        plain_profile_->membuf().size());
    }
    else
    {
      writer.version() = CURRENT_FREQ_CAP_PROFILE_VERSION;
    }

    PublisherAccountWriter pub_acc;
    pub_acc.timestamp() = timestamp.tv_sec;
    
    if (writer.publisher_accounts().size() != 0)
    {
      UserFreqCapProfileWriter::publisher_accounts_Container::iterator
        pa_it = writer.publisher_accounts().begin();
      std::set<unsigned long>::const_iterator in_it = publisher_account_ids.begin();

      while (pa_it != writer.publisher_accounts().end() &&
             in_it != publisher_account_ids.end())
      {
        if ((*pa_it).publisher_account_id() == *in_it)
        {
          (*pa_it).timestamp() = std::max((*pa_it).timestamp(), pub_acc.timestamp());

          ++pa_it;
          ++in_it;
        }
        else if ((*pa_it).publisher_account_id() < *in_it)
        {
          ++pa_it;
        }
        else
        {
          pub_acc.publisher_account_id() = *in_it;
          pa_it = writer.publisher_accounts().insert(pa_it, pub_acc);

          ++pa_it;
          ++in_it;
        }
      }

      while (in_it != publisher_account_ids.end())
      {
         pub_acc.publisher_account_id() = *in_it;
         writer.publisher_accounts().push_back(pub_acc);

         ++in_it;
      }
    }
    else
    {
      for (std::set<unsigned long>::const_iterator it = publisher_account_ids.begin();
           it != publisher_account_ids.end(); ++it)
      {
        pub_acc.publisher_account_id() = *it;
        writer.publisher_accounts().push_back(pub_acc);
      }
    }
    
    Generics::MemBuf mb(writer.size());
    plain_profile_->membuf().assign(mb.data(), mb.size());
    writer.save(plain_profile_->membuf().data(), plain_profile_->membuf().size());

    return true;
  }
  
  void
  UserFreqCapProfile::get_optin_publishers(
    std::list<unsigned long>& optin_publishers,
    const Generics::Time& time)
    /*throw(eh::Exception)*/
  {
    if(!plain_profile_->membuf().size())
    {
      return;
    }

    UserFreqCapProfileReader reader(
      plain_profile_->membuf().data(),
      plain_profile_->membuf().size());

    for(UserFreqCapProfileReader::publisher_accounts_Container::const_iterator it =
          reader.publisher_accounts().begin();
        it != reader.publisher_accounts().end(); ++it)
    {
      if ((*it).timestamp() >= time.tv_sec)
      {
        optin_publishers.push_back((*it).publisher_account_id());
      }
    }
  }

  void
  UserFreqCapProfile::print(
    std::ostream& out,
    const FreqCapConfig* /*fc_config*/)
    const /*throw(eh::Exception)*/
  {
    if(!plain_profile_->membuf().size())
    {
      return;
    }

    UserFreqCapProfileReader reader(
      plain_profile_->membuf().data(),
      plain_profile_->membuf().size());

    {
      unsigned long columns = sizeof(FC_TABLE_COLUMNS) / sizeof(FC_TABLE_COLUMNS[0]);

      Table table(columns);
      
      for(unsigned long i = 0; i < columns; i++)
      {
        table.column(i, FC_TABLE_COLUMNS[i]);
      }

      for(UserFreqCapProfileReader::freq_caps_Container::
            const_iterator fc_it = reader.freq_caps().begin();
          fc_it != reader.freq_caps().end(); ++fc_it)
      {
        Table::Row row(table.columns());
        row.add_field((*fc_it).fc_id());
        row.add_field(0);
        row.add_field((*fc_it).total_impressions());
        std::ostringstream ostr;
        for(FreqCapReader::last_impressions_Container::const_iterator imp_it =
              (*fc_it).last_impressions().begin();
            imp_it != (*fc_it).last_impressions().end(); ++imp_it)
        {
          if(imp_it != (*fc_it).last_impressions().begin())
          {
            ostr << ",";
          }
          ostr << Generics::Time(normalize_impression_timestamp(*imp_it)).get_gm_time() <<
            (is_uc_timestamp(*imp_it) ? "(unconfirmed)" : "(confirmed)");
        }
        row.add_field(ostr.str());
        table.add_row(row);
      }

      for(UserFreqCapProfileReader::virtual_freq_caps_Container::
            const_iterator fc_it = reader.virtual_freq_caps().begin();
          fc_it != reader.virtual_freq_caps().end(); ++fc_it)
      {
        Table::Row row(table.columns());
        row.add_field((*fc_it).fc_id());
        row.add_field(1);
        row.add_field((*fc_it).total_impressions());
        std::ostringstream ostr;
        for(FreqCapReader::last_impressions_Container::const_iterator imp_it =
              (*fc_it).last_impressions().begin();
            imp_it != (*fc_it).last_impressions().end(); ++imp_it)
        {
          if(imp_it != (*fc_it).last_impressions().begin())
          {
            ostr << ",";
          }
          ostr << Generics::Time(normalize_impression_timestamp(*imp_it)).get_gm_time() <<
            (is_uc_timestamp(*imp_it) ? "(unconfirmed)" : "(confirmed)");
        }
        row.add_field(ostr.str());
        table.add_row(row);
      }

      table.dump(out);
    }

    {
      unsigned long columns = sizeof(UCFC_TABLE_COLUMNS) / sizeof(UCFC_TABLE_COLUMNS[0]);

      Table table(columns);
      
      for(unsigned long i = 0; i < columns; i++)
      {
        table.column(i, UCFC_TABLE_COLUMNS[i]);
      }

      for(UserFreqCapProfileReader::uc_freq_caps_Container::
            const_iterator ufc_it = reader.uc_freq_caps().begin();
          ufc_it != reader.uc_freq_caps().end(); ++ufc_it)
      {
        Table::Row row(table.columns());
        row.add_field((*ufc_it).request_id());
        row.add_field(Generics::Time((*ufc_it).time()).gm_ft());

        {
          std::ostringstream ostr;
          Algs::print(ostr, (*ufc_it).freq_cap_ids().begin(), (*ufc_it).freq_cap_ids().end());
          row.add_field(ostr.str());
        }

        {
          std::ostringstream ostr;
          Algs::print(ostr, (*ufc_it).imps().begin(), (*ufc_it).imps().end());
          row.add_field(ostr.str());
        }

        table.add_row(row);
      }

      table.dump(out);
    }

    {
      unsigned long columns = sizeof(SEQ_ORDER_TABLE_COLUMNS) /
        sizeof(SEQ_ORDER_TABLE_COLUMNS[0]);

      Table table(columns);
      
      for(unsigned long i = 0; i < columns; i++)
      {
        table.column(i, SEQ_ORDER_TABLE_COLUMNS[i]);
      }

      for(UserFreqCapProfileReader::seq_orders_Container::
            const_iterator seq_order_it = reader.seq_orders().begin();
          seq_order_it != reader.seq_orders().end();
          ++seq_order_it)
      {
        Table::Row row(table.columns());
        row.add_field((*seq_order_it).ccg_id());
        row.add_field((*seq_order_it).set_id());
        row.add_field((*seq_order_it).imps());
        table.add_row(row);
      }

      table.dump(out);
    }

    {
      unsigned long columns = sizeof(PUBLISHER_ACCOUNTS_TABLE_COLUMNS) /
        sizeof(PUBLISHER_ACCOUNTS_TABLE_COLUMNS[0]);

      Table table(columns);

      for(unsigned long i = 0; i < columns; i++)
      {
        table.column(i, PUBLISHER_ACCOUNTS_TABLE_COLUMNS[i]);
      }

      for(UserFreqCapProfileReader::publisher_accounts_Container::const_iterator it =
            reader.publisher_accounts().begin();
          it != reader.publisher_accounts().end(); ++it)
      {
        Table::Row row(table.columns());
        row.add_field((*it).publisher_account_id());
        row.add_field(Generics::Time((*it).timestamp()).get_gm_time());
        table.add_row(row);
      }

      table.dump(out);
    }

    {
      unsigned long columns = sizeof(CAMPAIGN_FREQ_TABLE_COLUMNS) /
        sizeof(CAMPAIGN_FREQ_TABLE_COLUMNS[0]);

      Table table(columns);

      for(unsigned long i = 0; i < columns; i++)
      {
        table.column(i, CAMPAIGN_FREQ_TABLE_COLUMNS[i]);
      }

      const auto& campaign_freqs = reader.campaign_freqs();

      for (auto it = campaign_freqs.begin();
           it != campaign_freqs.end(); ++it)
      {
        Table::Row row(table.columns());
        row.add_field((*it).campaign_id());
        row.add_field((*it).imps());
        table.add_row(row);
      }

      table.dump(out);
    }
  }
}
}
