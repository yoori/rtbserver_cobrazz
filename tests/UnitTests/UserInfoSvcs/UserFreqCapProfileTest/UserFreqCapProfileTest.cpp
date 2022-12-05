#include <string>
#include <iostream>

#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>
#include <Commons/Algs.hpp>
#include <UserInfoSvcs/UserInfoCommons/Allocator.hpp>
#include <UserInfoSvcs/UserInfoCommons/UserFreqCapProfile.hpp>

#include "../../TestHelpers.hpp"

using namespace AdServer::UserInfoSvcs;
const bool TRACE_CONSIDER = 0;

bool
empty_full_fcs(
  UserFreqCapProfile& fc_profile,
  const Generics::Time& now,
  const FreqCapConfig& fc_config)
{
  UserFreqCapProfile::FreqCapIdList fcs;
  UserFreqCapProfile::SeqOrderList seq_orders;
  UserFreqCapProfile::CampaignFreqs campaign_freqs;
  fc_profile.full(fcs, 0, seq_orders, campaign_freqs, now, fc_config);
  return fcs.empty();
}

bool
fc_consider_and_period_check(
  UserFreqCapProfile& fc_profile,
  const char* STEP,
  const AdServer::Commons::RequestId& req_id,
  const Generics::Time& now,
  const UserFreqCapProfile::FreqCapIdList& fcs,
  const UserFreqCapProfile::FreqCapIdList& uc_fcs,
  const FreqCapConfig& fc_config,
  const Generics::Time& period)
{
  static const char* FUN = "fc_consider_and_period_check()";

  if(TRACE_CONSIDER)
  {
    std::cout << "Before consider at '" <<
      now.get_gm_time() << "':" << std::endl;
    fc_profile.print(std::cout, &fc_config);
  }

  fc_profile.consider(
    req_id,
    now,
    fcs,
    uc_fcs,
    UserFreqCapProfile::FreqCapIdList(),
    UserFreqCapProfile::SeqOrderList(),
    UserFreqCapProfile::CampaignIds(),
    UserFreqCapProfile::CampaignIds(),
    fc_config);

  if(TRACE_CONSIDER)
  {
    std::cout << "After consider at '" << now.get_gm_time() << "':" << std::endl;
    fc_profile.print(std::cout, &fc_config);
  }

  if(period != Generics::Time::ZERO)
  {
    if(empty_full_fcs(fc_profile, now + 1, fc_config))
    {
      std::cerr << FUN << ": fcs empty at " << STEP <<
        ", expected full by period for time = '" <<
        (now + 1).get_gm_time() << "': ";
      Algs::print(std::cerr, fcs.begin(), fcs.end());
      std::cerr << std::endl << "  profile:" << std::endl;
      fc_profile.print(std::cerr, &fc_config);
      std::cerr << std::endl;
      return false;
    }

    if(empty_full_fcs(fc_profile, now + period - 1, fc_config))
    {
      std::cerr << FUN << ": fcs empty at " << STEP <<
        ", expected full by period for time = '" <<
        (now + period - 1).get_gm_time() << "': ";
      Algs::print(std::cerr, fcs.begin(), fcs.end());
      std::cerr << std::endl << "  profile:" << std::endl;
      fc_profile.print(std::cerr, &fc_config);
      std::cerr << std::endl;
      return false;
    }
  }

  if(period != Generics::Time::ZERO &&
     !empty_full_fcs(fc_profile, now + period, fc_config))
  {
    std::cerr << FUN << ": fcs not empty at " << STEP << ": ";
    Algs::print(std::cerr, fcs.begin(), fcs.end());
    std::cerr << std::endl;
    return false;
  }

  return true;
}

int test_fc_full()
{
  static const char* FUN = "test_fc_full()";

  FreqCapConfig_var fc_config(new FreqCapConfig());
  fc_config->freq_caps.insert(
    std::make_pair(1, AdServer::Commons::FreqCap(
      1,
      10, // lifelimit
      Generics::Time(3), // period
      3, // window limit
      Generics::Time(20)  // window time
      )));

  SmartMemBuf_var buf(new SmartMemBuf);
  UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));

  {
    UserFreqCapProfile::FreqCapIdList fcs;
    fcs.push_back(1);

    if(!fc_consider_and_period_check(
         fc_profile,
         "step #1",
         AdServer::Commons::RequestId(),
         Generics::Time(1),
         fcs,
         UserFreqCapProfile::FreqCapIdList(),
         *fc_config,
         Generics::Time(3)))
    {
      return 1;
    }

    if(!fc_consider_and_period_check(
         fc_profile,
         "step #2",
         AdServer::Commons::RequestId(),
         Generics::Time(4),
         fcs,
         UserFreqCapProfile::FreqCapIdList(),
         *fc_config,
         Generics::Time(3)))
    {
      return 1;
    }

    {
      fc_profile.consider(
        AdServer::Commons::RequestId(),
        Generics::Time(7),
        fcs,
        UserFreqCapProfile::FreqCapIdList(),
        UserFreqCapProfile::FreqCapIdList(),
        UserFreqCapProfile::SeqOrderList(),
        UserFreqCapProfile::CampaignIds(),
        UserFreqCapProfile::CampaignIds(),
        *fc_config);

      UserFreqCapProfile::FreqCapIdList check_fcs;
      UserFreqCapProfile::SeqOrderList seq_orders;
      UserFreqCapProfile::CampaignFreqs campaign_freqs;
      fc_profile.full(check_fcs, 0, seq_orders, campaign_freqs, Generics::Time(10), *fc_config);
      if(check_fcs.size() != 1 || *check_fcs.begin() != 1)
      {
        std::cerr << FUN << ": unexpected fcs at step #3, "
          "expected full by window for time = '" <<
          Generics::Time(10) << "': ";
        Algs::print(std::cerr, check_fcs.begin(), check_fcs.end());
        std::cerr << std::endl << "  profile:" << std::endl;
        fc_profile.print(std::cerr, fc_config);
        std::cerr << std::endl;
        return 1;
      }
    }

    {
      UserFreqCapProfile::FreqCapIdList check_fcs;
      UserFreqCapProfile::SeqOrderList seq_orders;
      UserFreqCapProfile::CampaignFreqs campaign_freqs;
      fc_profile.full(check_fcs, 0, seq_orders, campaign_freqs, Generics::Time(20), *fc_config);
      if(check_fcs.size() != 1 || *check_fcs.begin() != 1)
      {
        std::cerr << FUN << ": unexpected fcs at step #4, "
          "expected full by window for time = '" <<
          Generics::Time(20) << "': ";
        Algs::print(std::cerr, check_fcs.begin(), check_fcs.end());
        std::cerr << std::endl << "  profile:" << std::endl;
        fc_profile.print(std::cerr, fc_config);
        std::cerr << std::endl;
        return 1;
      }
    }

    {
      UserFreqCapProfile::FreqCapIdList check_fcs;
      UserFreqCapProfile::SeqOrderList seq_orders;
      UserFreqCapProfile::CampaignFreqs campaign_freqs;
      fc_profile.full(check_fcs, 0, seq_orders, campaign_freqs, Generics::Time(22), *fc_config);
      if(!check_fcs.empty())
      {
        std::cerr << FUN << ": not empty fcs at step #5, "
          "for time = '" <<
          Generics::Time(22) << "': ";
        Algs::print(std::cerr, check_fcs.begin(), check_fcs.end());
        std::cerr << std::endl << "  profile:" << std::endl;
        fc_profile.print(std::cerr, fc_config);
        std::cerr << std::endl;
        return 1;
      }
    }
  }

  return 0;
}

int test_ucfc()
{
  static const char* FUN = "test_ucfc()";

  FreqCapConfig_var fc_config(new FreqCapConfig());
  fc_config->confirm_timeout = Generics::Time(5);

  fc_config->freq_caps.insert(
    std::make_pair(1, AdServer::Commons::FreqCap(
      1,
      1, // lifelimit
      Generics::Time::ZERO, // period
      0, // window limit
      Generics::Time::ZERO  // window time
      )));

  fc_config->freq_caps.insert(
    std::make_pair(2, AdServer::Commons::FreqCap(
      2,
      1, // lifelimit
      Generics::Time::ZERO, // period
      0, // window limit
      Generics::Time::ZERO  // window time
      )));

  fc_config->freq_caps.insert(
    std::make_pair(3, AdServer::Commons::FreqCap(
      3,
      2, // lifelimit
      Generics::Time::ZERO, // period
      0, // window limit
      Generics::Time::ZERO  // window time
      )));

  {
    SmartMemBuf_var buf(new SmartMemBuf);
    UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));

    UserFreqCapProfile::FreqCapIdList fcs;
    fcs.push_back(1);

    UserFreqCapProfile::FreqCapIdList fcs2;
    fcs2.push_back(2);

    if(!fc_consider_and_period_check(
         fc_profile,
         "step #1",
         AdServer::Commons::RequestId::create_random_based(),
         Generics::Time(1),
         UserFreqCapProfile::FreqCapIdList(),
         fcs,
         *fc_config,
         Generics::Time::ZERO))
    {
      return 1;
    }

    if(!fc_consider_and_period_check(
         fc_profile,
         "step #2",
         AdServer::Commons::RequestId::create_random_based(),
         Generics::Time(1),
         UserFreqCapProfile::FreqCapIdList(),
         fcs2,
         *fc_config,
         Generics::Time::ZERO))
    {
      return 1;
    }

    {
      UserFreqCapProfile::FreqCapIdList check_fcs;
      UserFreqCapProfile::SeqOrderList seq_orders;
      UserFreqCapProfile::CampaignFreqs campaign_freqs;
      fc_profile.full(check_fcs, 0, seq_orders, campaign_freqs, Generics::Time(7), *fc_config);
      if(!check_fcs.empty())
      {
        std::cerr << FUN << ": not empty fcs at step #3, "
          "for time = '" <<
          Generics::Time(22) << "': ";
        Algs::print(std::cerr, check_fcs.begin(), check_fcs.end());
        std::cerr << std::endl << "  profile:" << std::endl;
        fc_profile.print(std::cerr, fc_config);
        std::cerr << std::endl;
        return 1;
      }
    }

    {
      AdServer::Commons::RequestId req_id =
        AdServer::Commons::RequestId::create_random_based();

      fc_profile.consider(
        req_id,
        Generics::Time(25),
        UserFreqCapProfile::FreqCapIdList(),
        fcs, // unconfirmed
        UserFreqCapProfile::FreqCapIdList(),
        UserFreqCapProfile::SeqOrderList(),
        UserFreqCapProfile::CampaignIds(),
        UserFreqCapProfile::CampaignIds(),
        *fc_config);

      fc_profile.consider(
        AdServer::Commons::RequestId::create_random_based(),
        Generics::Time(26),
        UserFreqCapProfile::FreqCapIdList(),
        fcs2, // unconfirmed
        UserFreqCapProfile::FreqCapIdList(),
        UserFreqCapProfile::SeqOrderList(),
        UserFreqCapProfile::CampaignIds(),
        UserFreqCapProfile::CampaignIds(),
        *fc_config);

      fc_profile.confirm_request(
        req_id,
        Generics::Time(26),
        *fc_config);

      UserFreqCapProfile::FreqCapIdList check_fcs;
      UserFreqCapProfile::SeqOrderList seq_orders;
      UserFreqCapProfile::CampaignFreqs campaign_freqs;
      fc_profile.full(check_fcs, 0, seq_orders, campaign_freqs, Generics::Time(26 + 6), *fc_config);
      if(check_fcs.size() != 1 || *check_fcs.begin() != 1)
      {
        std::cerr << FUN << ": unexpected fcs at step #4, "
          "expected full by window for time = '" <<
          Generics::Time(26 + 6) << "': ";
        Algs::print(std::cerr, check_fcs.begin(), check_fcs.end());
        std::cerr << std::endl << "  profile:" << std::endl;
        fc_profile.print(std::cerr, fc_config);
        std::cerr << std::endl;
        return 1;
      }
    }
  }

  {
    SmartMemBuf_var buf(new SmartMemBuf);
    UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));

    UserFreqCapProfile::FreqCapIdList fcs2;
    fcs2.push_back(2);

    UserFreqCapProfile::FreqCapIdList fcs3;
    fcs3.push_back(3);

    // two unconfirmed imps at one freq cap - one confirmed after
    AdServer::Commons::RequestId req_id =
      AdServer::Commons::RequestId::create_random_based();
    AdServer::Commons::RequestId req_id2 =
      AdServer::Commons::RequestId::create_random_based();

    fc_profile.consider(
      req_id,
      Generics::Time(25),
      UserFreqCapProfile::FreqCapIdList(),
      fcs3, // unconfirmed
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::SeqOrderList(),
      UserFreqCapProfile::CampaignIds(),
      UserFreqCapProfile::CampaignIds(),
      *fc_config);

    fc_profile.consider(
      AdServer::Commons::RequestId::create_random_based(),
      Generics::Time(26),
      UserFreqCapProfile::FreqCapIdList(),
      fcs2, // unconfirmed
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::SeqOrderList(),
      UserFreqCapProfile::CampaignIds(),
      UserFreqCapProfile::CampaignIds(),
      *fc_config);

    fc_profile.consider(
      req_id2,
      Generics::Time(29),
      UserFreqCapProfile::FreqCapIdList(),
      fcs3, // unconfirmed
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::SeqOrderList(),
      UserFreqCapProfile::CampaignIds(),
      UserFreqCapProfile::CampaignIds(),
      *fc_config);

    fc_profile.confirm_request(
      req_id,
      Generics::Time(29),
      *fc_config);

    UserFreqCapProfile::FreqCapIdList check_fcs;
    UserFreqCapProfile::SeqOrderList seq_orders;
    UserFreqCapProfile::CampaignFreqs campaign_freqs;
    fc_profile.full(check_fcs, 0, seq_orders, campaign_freqs, Generics::Time(25 + 6), *fc_config);
    if(check_fcs.size() != 2 || *check_fcs.begin() != 2 ||
       *++check_fcs.begin() != 3)
    {
      std::cerr << FUN << ": unexpected fcs at step #5, "
        "expected full by total for time = '" <<
        Generics::Time(25 + 6) << "': ";
      Algs::print(std::cerr, check_fcs.begin(), check_fcs.end());
      std::cerr << std::endl << "  profile:" << std::endl;
      fc_profile.print(std::cerr, fc_config);
      std::cerr << std::endl;
      return 1;
    }

    fc_profile.confirm_request(
      req_id2,
      Generics::Time(25 + 6),
      *fc_config);

    check_fcs.clear();
    fc_profile.full(check_fcs, 0, seq_orders, campaign_freqs, Generics::Time(29 + 6), *fc_config);
    if(check_fcs.size() != 1 || *check_fcs.begin() != 3)
    {
      std::cerr << FUN << ": unexpected fcs at step #6, "
        "expected full by total for time = '" <<
        Generics::Time(29 + 6) << "': ";
      Algs::print(std::cerr, check_fcs.begin(), check_fcs.end());
      std::cerr << std::endl << "  profile:" << std::endl;
      fc_profile.print(std::cerr, fc_config);
      std::cerr << std::endl;
      return 1;
    }

    // fc_profile.print(std::cerr, fc_config);
  }

  return 0;
}

bool
check_fcs(
  const String::SubString& STEP,
  const UserFreqCapProfile::FreqCapIdList& check_fcs,
  const UserFreqCapProfile::FreqCapIdList& etalon)
{
  if(check_fcs.size() != etalon.size() ||
    !std::equal(check_fcs.begin(), check_fcs.end(), etalon.begin()))
  {
    std::cerr << STEP << ": unexpected fcs: (";
    Algs::print(std::cerr, check_fcs.begin(), check_fcs.end());
    std::cerr << ") instead (";
    Algs::print(std::cerr, etalon.begin(), etalon.end());
    std::cerr << ")" << std::endl;
    return false;
  }

  return true;
}

int
concurrent_case_ADSC_9494()
{
  const std::string FUN = "concurrent_case_ADSC_9494()";

  FreqCapConfig_var fc_config(new FreqCapConfig());
  fc_config->confirm_timeout = Generics::Time(60);

  fc_config->freq_caps.insert(
    std::make_pair(1, AdServer::Commons::FreqCap(
      1,
      0, // lifelimit
      Generics::Time(900), // period(= 900 sec)
      3, // window limit
      Generics::Time::ONE_DAY // window time
      )));

  try
  {
    // see ADSC-9494 comment
    SmartMemBuf_var buf(new SmartMemBuf);
    UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));

    const Generics::Time BASE(0);

    for(int i = 0; i < 2; ++i)
    {
      // step 1: consider request
      UserFreqCapProfile::FreqCapIdList fcs;
      fcs.push_back(1);

      // two unconfirmed imps at one freq cap - one confirmed after
      AdServer::Commons::RequestId req_id =
        AdServer::Commons::RequestId::create_random_based();

      fc_profile.consider(
        req_id,
        BASE,
        UserFreqCapProfile::FreqCapIdList(),
        fcs, // unconfirmed
        UserFreqCapProfile::FreqCapIdList(),
        UserFreqCapProfile::SeqOrderList(),
        UserFreqCapProfile::CampaignIds(),
        UserFreqCapProfile::CampaignIds(),
        *fc_config);

      fc_profile.confirm_request(req_id, BASE, *fc_config);
    }

    for(int i = 0; i < 2; ++i)
    {
      // step 2: consider concurrent request
      UserFreqCapProfile::FreqCapIdList fcs;
      fcs.push_back(1);

      // two unconfirmed imps at one freq cap - one confirmed after
      AdServer::Commons::RequestId req_id =
        AdServer::Commons::RequestId::create_random_based();

      fc_profile.consider(
        req_id,
        BASE + 1,
        UserFreqCapProfile::FreqCapIdList(),
        fcs, // unconfirmed
        UserFreqCapProfile::FreqCapIdList(),
        UserFreqCapProfile::SeqOrderList(),
        UserFreqCapProfile::CampaignIds(),
        UserFreqCapProfile::CampaignIds(),
        *fc_config);

      if(i == 1)
      {
        fc_profile.confirm_request(req_id, BASE, *fc_config);
      }

      // profile contains BASE, BASE + 1, BASE + 1 timestamps if impl have error
      // BASE, BASE, BASE + 1(unconfirmed), BASE + 1
    }

    {
      // step 3: initiate last_impressions cleanup
      UserFreqCapProfile::FreqCapIdList etalon_fcs;
      etalon_fcs.push_back(1);
      UserFreqCapProfile::FreqCapIdList full_fcs;
      UserFreqCapProfile::SeqOrderList skip;
      UserFreqCapProfile::CampaignFreqs campaign_freqs;
      fc_profile.full(
        full_fcs,
        0,
        skip,
        campaign_freqs,
        BASE + 1 + 60 + 1,
        *fc_config);
      if(!check_fcs(FUN + ": step 2", full_fcs, etalon_fcs))
      {
        return 1;
      }
    }

    fc_profile.print(std::cout, fc_config);
  }
  catch(const eh::Exception& ex)
  {
    assert(0);
  }

  return 0;
}

int
test_campaign_freqs()
{
  FreqCapConfig_var fc_config(new FreqCapConfig());
  fc_config->confirm_timeout = Generics::Time(60);
  fc_config->campaign_ids.insert(100);
  fc_config->campaign_ids.insert(300);

  SmartMemBuf_var buf(new SmartMemBuf);

  const AdServer::Commons::RequestId req_id =
    AdServer::Commons::RequestId::create_random_based();
  const Generics::Time BASE(0);

  {
    UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));
    fc_profile.consider(
      req_id,
      BASE,
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::SeqOrderList(),
      UserFreqCapProfile::CampaignIds(),
      UserFreqCapProfile::CampaignIds{200, 100},
      *fc_config);

    buf = Algs::copy_membuf(fc_profile.transfer_membuf());

    const UserFreqCapProfileReader reader(
      buf->membuf().data(),
      buf->membuf().size());

    ASSERT_EQUALS (reader.uc_freq_caps().size(), 1U);
    ASSERT_EQUALS (reader.freq_caps().size(), 0U);
    ASSERT_EQUALS (reader.virtual_freq_caps().size(), 0U);
    ASSERT_EQUALS (reader.seq_orders().size(), 0U);
    ASSERT_EQUALS (reader.publisher_accounts().size(), 0U);
    ASSERT_EQUALS (reader.campaign_freqs().size(), 0U);

    const auto imps = (*reader.uc_freq_caps().begin()).imps();
    ASSERT_EQUALS (imps.size(), 2U);
    auto it = imps.begin();
    ASSERT_EQUALS (*it, 200U);
    ++it;
    ASSERT_EQUALS (*it, 100U);
  }

  {
    UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));
    fc_profile.confirm_request(req_id, BASE + 26, *fc_config);
    buf = Algs::copy_membuf(fc_profile.transfer_membuf());

    const UserFreqCapProfileReader reader(
      buf->membuf().data(),
      buf->membuf().size());

    ASSERT_EQUALS (reader.uc_freq_caps().size(), 0U);
    ASSERT_EQUALS (reader.freq_caps().size(), 0U);
    ASSERT_EQUALS (reader.virtual_freq_caps().size(), 0U);
    ASSERT_EQUALS (reader.seq_orders().size(), 0U);
    ASSERT_EQUALS (reader.publisher_accounts().size(), 0U);
    ASSERT_EQUALS (reader.campaign_freqs().size(), 1U);

    const auto campaign_freqs = reader.campaign_freqs();
    ASSERT_EQUALS (campaign_freqs.size(), 1U);
    auto it = campaign_freqs.begin();
    ASSERT_EQUALS ((*it).campaign_id(), 100U);
    ASSERT_EQUALS ((*it).imps(), 1U);
  }

  {
    UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));
    UserFreqCapProfile::FreqCapIdList full_fcs;
    UserFreqCapProfile::SeqOrderList seq_orders;
    UserFreqCapProfile::CampaignFreqs campaign_freqs;

    fc_profile.full(
      full_fcs,
      0,
      seq_orders,
      campaign_freqs,
      BASE + 1,
      *fc_config);

    ASSERT_EQUALS (campaign_freqs.size(), 1U);
    auto it = campaign_freqs.begin();
    ASSERT_EQUALS ((*it).campaign_id, 100U);
    ASSERT_EQUALS ((*it).imps, 1U);
    buf = Algs::copy_membuf(fc_profile.transfer_membuf());
  }

  {
    UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));
    fc_profile.consider(
      req_id,
      BASE,
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::SeqOrderList(),
      UserFreqCapProfile::CampaignIds(),
      UserFreqCapProfile::CampaignIds{200, 100},
      *fc_config);

    UserFreqCapProfile::FreqCapIdList full_fcs;
    UserFreqCapProfile::SeqOrderList seq_orders;
    UserFreqCapProfile::CampaignFreqs campaign_freqs;

    fc_profile.full(
      full_fcs,
      0,
      seq_orders,
      campaign_freqs,
      BASE + 120,
      *fc_config);

    ASSERT_EQUALS (campaign_freqs.size(), 1U);
    auto it = campaign_freqs.begin();
    ASSERT_EQUALS ((*it).campaign_id, 100U);
    ASSERT_EQUALS ((*it).imps, 1U);

    buf = Algs::copy_membuf(fc_profile.transfer_membuf());
  }

  SmartMemBuf_var buf2(new SmartMemBuf);

  {
    UserFreqCapProfile fc_profile2(Generics::transfer_membuf(buf2));
    fc_profile2.consider(
      req_id,
      BASE,
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::SeqOrderList(),
      UserFreqCapProfile::CampaignIds{300, 100},
      UserFreqCapProfile::CampaignIds(),
      *fc_config);

    fc_profile2.consider(
      req_id,
      BASE,
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::FreqCapIdList(),
      UserFreqCapProfile::SeqOrderList(),
      UserFreqCapProfile::CampaignIds{100},
      UserFreqCapProfile::CampaignIds(),
      *fc_config);

    buf2 = Algs::copy_membuf(fc_profile2.transfer_membuf());

    UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));
    fc_profile.merge(Generics::transfer_membuf(buf2), BASE + 30, *fc_config);

    UserFreqCapProfile::FreqCapIdList full_fcs;
    UserFreqCapProfile::SeqOrderList seq_orders;
    UserFreqCapProfile::CampaignFreqs campaign_freqs;

    fc_profile.full(
      full_fcs,
      0,
      seq_orders,
      campaign_freqs,
      BASE + 1,
      *fc_config);

    ASSERT_EQUALS (campaign_freqs.size(), 2U);
    auto it = campaign_freqs.begin();
    ASSERT_EQUALS ((*it).campaign_id, 100U);
    ASSERT_EQUALS ((*it).imps, 3U);
    ++it;
    ASSERT_EQUALS ((*it).campaign_id, 300U);
    ASSERT_EQUALS ((*it).imps, 1U);
  }

  return 0;
}

int
main(
  int /*argc*/,
  char** /*argv*/)
  noexcept
{
  int ret = 0;
  try
  {
    /*
    SmartMemBuf_var buf(new SmartMemBuf);
    buf->membuf().assign(ARR, sizeof(ARR));
    UserFreqCapProfile fc_profile(Generics::transfer_membuf(buf));

    FreqCapConfig_var fc_config(new FreqCapConfig());
    fc_config->confirm_timeout = Generics::Time(5);
    fc_config->freq_caps.insert(
      std::make_pair(117737, AdServer::Commons::FreqCap(
        117737,
        0, // lifelimit
        Generics::Time(1800), // period
        3, // window limit
        Generics::Time(3600)  // window time
        )));

    UserFreqCapProfile::FreqCapIdList fcs;
    UserFreqCapProfile::SeqOrderList seq_orders;
    Generics::Time now = Generics::Time(1435915007 + 9000) + Generics::Time::ONE_DAY * 10;
    std::cerr << now.gm_ft() << std::endl;

    fc_profile.confirm_request(
      AdServer::Commons::RequestId("7zHiWmj7Rf-FPvTDw8v11w.."),
      now,
      *fc_config);

    //std::cout << "SIZE: " << fc_profile.transfer_membuf()->membuf().size() << std::endl;

    //fc_profile.full(fcs, 0, seq_orders, Generics::Time(1435915007 + 9000), *fc_config);

    fc_profile.print(std::cout, fc_config);
    */

    ret += test_fc_full();
    ret += test_ucfc();
    ret += concurrent_case_ADSC_9494();
    ret += test_campaign_freqs();

    return ret;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return -1;
}
