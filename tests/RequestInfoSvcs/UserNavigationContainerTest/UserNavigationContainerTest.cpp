#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <unistd.h>

#include <Commons/Algs.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>
#include <Logger/Logger.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserNavigationProfileAdapter.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserNavigationProfile_v1.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserNavigationProfile_v2.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/UserNavigationContainer.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/UserNavigationProfileJson.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserNavigationProfile.hpp>

namespace
{
  using AdServer::RequestInfoSvcs::UserNavigationContainer;

  struct ExpectedNavigation
  {
    Generics::Time date;
    std::string url;
    std::uint64_t count;
  };

  void
  process(
    UserNavigationContainer* container,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& time,
    std::string_view url)
  {
    UserNavigationContainer::RequestInfo request_info;
    request_info.user_id = user_id;
    request_info.time = time;
    request_info.urls.push_back(url);
    AdServer::Commons::sync_wait(container->co_process_request(request_info));
  }

  void
  process(
    UserNavigationContainer* container,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& time,
    std::vector<std::string_view> urls)
  {
    UserNavigationContainer::RequestInfo request_info;
    request_info.user_id = user_id;
    request_info.time = time;
    request_info.urls = std::move(urls);
    AdServer::Commons::sync_wait(container->co_process_request(request_info));
  }

  AdServer::Commons::StartableAwaitable<Generics::ConstSmartMemBuf_var>
  get_profile(
    UserNavigationContainer* container,
    const AdServer::Commons::UserId& user_id,
    std::optional<std::uint32_t> date = std::nullopt)
  {
    co_return co_await container->co_get_profile(user_id, date);
  }

  void
  check_profile_data(
    const Generics::ConstSmartMemBuf* profile,
    const std::vector<ExpectedNavigation>& expected,
    const std::vector<ExpectedNavigation>& expected_keywords = {})
  {
    const AdServer::RequestInfoSvcs::UserNavigationProfileReader reader(
      profile->membuf().data(),
      profile->membuf().size());

    std::set<std::uint32_t> expected_days;
    for (const auto& navigation : expected)
    {
      expected_days.insert(navigation.date.tv_sec);
    }
    for (const auto& keyword : expected_keywords)
    {
      expected_days.insert(keyword.date.tv_sec);
    }

    if (reader.days().size() != expected_days.size())
    {
      throw std::runtime_error("Unexpected navigation day count");
    }

    auto expected_navigation = expected.begin();
    for (const auto day : reader.days())
    {
      for (const auto navigation : day.navigations())
      {
        if (expected_navigation == expected.end() ||
          day.date() != expected_navigation->date.tv_sec ||
          navigation.url() != expected_navigation->url ||
          navigation.count() != expected_navigation->count)
        {
          throw std::runtime_error("Unexpected navigation entry");
        }

        ++expected_navigation;
      }
    }

    if (expected_navigation != expected.end())
    {
      throw std::runtime_error("Unexpected navigation count");
    }
    auto expected_keyword = expected_keywords.begin();
    for (const auto day : reader.days())
    {
      for (const auto keyword : day.page_keywords())
      {
        if (expected_keyword == expected_keywords.end() ||
          day.date() != expected_keyword->date.tv_sec ||
          keyword.keyword() != expected_keyword->url || keyword.count() != expected_keyword->count)
        {
          throw std::runtime_error("Unexpected page keyword entry");
        }
        ++expected_keyword;
      }
    }
    if (expected_keyword != expected_keywords.end())
    {
      throw std::runtime_error("Unexpected page keyword count");
    }
  }

  void
  check_profile(
    UserNavigationContainer* container,
    const AdServer::Commons::UserId& user_id,
    const std::vector<ExpectedNavigation>& expected,
    std::optional<std::uint32_t> date = std::nullopt,
    const std::vector<ExpectedNavigation>& expected_keywords = {})
  {
    const Generics::ConstSmartMemBuf_var profile = AdServer::Commons::sync_wait(
      get_profile(container, user_id, date));
    if (!profile.in())
    {
      throw std::runtime_error("Profile is absent");
    }

    check_profile_data(profile, expected, expected_keywords);
  }

  void
  check_v1_adapter()
  {
    AdServer::RequestInfoSvcs_v1::UserNavigationProfileWriter old_profile;
    old_profile.version() = 1;
    for (const auto& navigation_info : std::vector<ExpectedNavigation>{
      {Generics::Time(10), "a", 1},
      {Generics::Time(10), "b", 2},
      {Generics::Time(10), "rtbyob\x01", 7},
      {Generics::Time(20), "c", 3},
      {Generics::Time(20), "poadnoref", 9}})
    {
      AdServer::RequestInfoSvcs_v1::NavigationWriter navigation;
      navigation.date() = navigation_info.date.tv_sec;
      navigation.url() = navigation_info.url;
      navigation.count() = navigation_info.count;
      old_profile.navigations().push_back(std::move(navigation));
    }

    Generics::SmartMemBuf_var old_mem_buf(new Generics::SmartMemBuf(old_profile.size()));
    old_profile.save(old_mem_buf->membuf().data(), old_mem_buf->membuf().size());

    const AdServer::RequestInfoSvcs::UserNavigationProfileAdapter adapter;
    const Generics::ConstSmartMemBuf_var old_const_mem_buf =
      Generics::transfer_membuf(old_mem_buf);
    const Generics::ConstSmartMemBuf_var profile = adapter(old_const_mem_buf.in());
    const AdServer::RequestInfoSvcs::UserNavigationProfileReader reader(
      profile->membuf().data(),
      profile->membuf().size());
    if (reader.version() !=
        AdServer::RequestInfoSvcs::CURRENT_USER_NAVIGATION_PROFILE_VERSION ||
      reader.days().size() != 2)
    {
      throw std::runtime_error("Unexpected adapted profile structure");
    }

    check_profile_data(
      profile,
      {
        {Generics::Time(10), "a", 1},
        {Generics::Time(10), "b", 2},
        {Generics::Time(20), "c", 3}
      },
      {
        {Generics::Time(10), "rtbyob\x01", 7},
        {Generics::Time(20), "poadnoref", 9}
      });
  }

  void check_v2_adapter()
  {
    AdServer::RequestInfoSvcs_v2::UserNavigationProfileWriter old_profile;
    old_profile.version() = 2;
    AdServer::RequestInfoSvcs_v2::NavigationDayWriter day;
    day.date() = 10;
    const std::vector<ExpectedNavigation> old_entries = {
      {Generics::Time(10), "https://rtb.example/", 1},
      {Generics::Time(10), "ordinary-keyword", 2},
      {Generics::Time(10), "poadnoref", 3},
      {Generics::Time(10), "rtbyob\x01", 4}};
    for (const auto& entry : old_entries)
    {
      AdServer::RequestInfoSvcs_v2::NavigationWriter navigation;
      navigation.url() = entry.url;
      navigation.count() = entry.count;
      day.navigations().push_back(std::move(navigation));
    }
    old_profile.days().push_back(std::move(day));
    Generics::SmartMemBuf_var buffer(new Generics::SmartMemBuf(old_profile.size()));
    old_profile.save(buffer->membuf().data(), buffer->membuf().size());
    const auto old_buffer = Generics::transfer_membuf(buffer);
    const AdServer::RequestInfoSvcs::UserNavigationProfileAdapter adapter;
    const auto converted = adapter(old_buffer.in());
    check_profile_data(converted, {old_entries[0], old_entries[1]},
      {old_entries[2], old_entries[3]});
    const AdServer::RequestInfoSvcs::UserNavigationProfileReader reader(
      converted->membuf().data(), converted->membuf().size());
    std::string json;
    {
      AdServer::Commons::JsonFormatter root(json);
      AdServer::Commons::JsonObject object(root.add_object("profile"));
      AdServer::RequestInfoSvcs::append_navigation_json(object, &reader);
    }
    const std::size_t keyword_position = json.find("\"page_keywords\"");
    if (keyword_position == std::string::npos ||
      json.substr(0, keyword_position).find("poadnoref") != std::string::npos ||
      json.substr(keyword_position).find("ordinary-keyword") != std::string::npos ||
      json.find("\"keyword\": \"poadnoref\"") == std::string::npos ||
      json.find("rtbyob\\u0001") == std::string::npos)
    {
      throw std::runtime_error("HTTP profile lost types or JSON escaping: " + json);
    }
    json.clear();
    {
      AdServer::Commons::JsonFormatter root(json);
      AdServer::Commons::JsonObject object(root.add_object("profile"));
      AdServer::RequestInfoSvcs::append_navigation_json(object, nullptr);
    }
    if (json != "{\"profile\": {\"version\": 3, \"urls\": [], \"page_keywords\": []}}")
    {
      throw std::runtime_error("Unexpected empty HTTP profile: " + json);
    }
    const auto repeated = adapter(converted.in());
    if (repeated.in() != converted.in())
    {
      throw std::runtime_error("Current profile was unnecessarily converted");
    }
  }

  void check_typed_profile(UserNavigationContainer* container, const Generics::Time& current_day)
  {
    const Generics::Time today = current_day - Generics::Time::ONE_DAY;
    UserNavigationContainer::RequestInfo request;
    request.user_id = AdServer::Commons::UserId::create_random_based();
    request.time = today - Generics::Time::ONE_DAY;
    request.urls = {"rtb.example", "same", ""};
    request.page_keywords = {"plain-keyword", "same", ""};
    AdServer::Commons::sync_wait(container->co_process_request(request));
    request.urls.clear();
    request.page_keywords = {"same"};
    AdServer::Commons::sync_wait(container->co_process_request(request));
    auto profile = AdServer::Commons::sync_wait(get_profile(container, request.user_id));
    check_profile_data(profile,
      {{request.time, "rtb.example", 1}, {request.time, "same", 1}},
      {{request.time, "plain-keyword", 1}, {request.time, "same", 2}});

    request.time = today;
    request.page_keywords = {"new"};
    AdServer::Commons::sync_wait(container->co_process_request(request));
    profile = AdServer::Commons::sync_wait(get_profile(container, request.user_id));
    check_profile_data(profile,
      {{today - Generics::Time::ONE_DAY, "rtb.example", 1},
       {today - Generics::Time::ONE_DAY, "same", 1}},
      {{today - Generics::Time::ONE_DAY, "same", 2}, {today, "new", 1}});

    request.time = today + Generics::Time::ONE_DAY;
    request.page_keywords = {"a", "b", "c", "d"};
    AdServer::Commons::sync_wait(container->co_process_request(request));
    profile = AdServer::Commons::sync_wait(get_profile(container, request.user_id));
    check_profile_data(profile, {}, {{request.time, "a", 1}, {request.time, "b", 1},
      {request.time, "c", 1}, {request.time, "d", 1}});
  }

  void
  save_legacy_profile(
    const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& date)
  {
    auto user_map = AdServer::ProfilingCommons::ProfileMapFactory::
      open_rocksdb_chunked_map<
        AdServer::Commons::UserId,
        AdServer::ProfilingCommons::UserIdAccessor,
        unsigned long (*)(const Generics::Uuid&)>(
          1,
          chunk_folders,
          "UserNavigation",
          AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
            Generics::Time::ONE_DAY * 30),
          AdServer::Commons::uuid_distribution_hash);
    user_map.second->activate_object();

    AdServer::RequestInfoSvcs_v2::UserNavigationProfileWriter profile;
    profile.version() = 2;
    AdServer::RequestInfoSvcs_v2::NavigationDayWriter day;
    day.date() = date.tv_sec;
    AdServer::RequestInfoSvcs_v2::NavigationWriter navigation;
    navigation.url() = "legacy";
    navigation.count() = 1;
    day.navigations().push_back(std::move(navigation));
    AdServer::RequestInfoSvcs_v2::NavigationWriter keyword;
    keyword.url() = "poadlegacy";
    keyword.count() = 5;
    day.navigations().push_back(std::move(keyword));
    profile.days().push_back(std::move(day));

    Generics::SmartMemBuf_var mem_buf(new Generics::SmartMemBuf(profile.size()));
    profile.save(mem_buf->membuf().data(), mem_buf->membuf().size());
    const Generics::ConstSmartMemBuf_var const_mem_buf = Generics::transfer_membuf(mem_buf);
    user_map.first->save_profile(user_id, const_mem_buf.in(), date);

    user_map.second->deactivate_object();
    user_map.second->wait_object();
  }

  bool
  legacy_profile_exists(
    const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
    const AdServer::Commons::UserId& user_id)
  {
    auto user_map = AdServer::ProfilingCommons::ProfileMapFactory::
      open_rocksdb_chunked_map<
        AdServer::Commons::UserId,
        AdServer::ProfilingCommons::UserIdAccessor,
        unsigned long (*)(const Generics::Uuid&)>(
          1,
          chunk_folders,
          "UserNavigation",
          AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
            Generics::Time::ONE_DAY * 30),
          AdServer::Commons::uuid_distribution_hash);
    user_map.second->activate_object();
    const bool result = user_map.first->check_profile(user_id);
    user_map.second->deactivate_object();
    user_map.second->wait_object();
    return result;
  }
}

int
main()
{
  constexpr std::size_t USER_NAVIGATIONS_LIMIT = 4;
  constexpr unsigned long USER_NAVIGATION_PERIOD_DAYS = 10;

  const std::filesystem::path root = std::filesystem::temp_directory_path() /
    ("UserNavigationContainerTest-" + std::to_string(::getpid()));

  try
  {
    check_v1_adapter();
    check_v2_adapter();

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "Chunk_0_1");

    AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_folders;
    AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
      chunk_folders,
      root.c_str(),
      "Chunk");

    const AdServer::Commons::UserId legacy_user_id =
      AdServer::Commons::UserId::create_random_based();
    const Generics::Time today = Algs::round_to_day(Generics::Time::get_time_of_day());
    save_legacy_profile(chunk_folders, legacy_user_id, today);

    Logging::Logger_var logger = new Logging::Null::Logger;
    AdServer::RequestInfoSvcs::UserNavigationContainer_var container =
      new UserNavigationContainer(
        logger,
        1,
        chunk_folders,
        "UserNavigation",
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          1024 * 1024,
          1024 * 1024,
          2 * 1024 * 1024,
          20,
          Generics::Time::ONE_DAY * 30),
        USER_NAVIGATIONS_LIMIT,
        USER_NAVIGATION_PERIOD_DAYS);
    container->activate_object();

    check_typed_profile(container, today);

    const AdServer::Commons::UserId user_id =
      AdServer::Commons::UserId::create_random_based();
    const AdServer::Commons::UserId empty_user_id =
      AdServer::Commons::UserId::create_random_based();

    check_profile(container, legacy_user_id, {{today, "legacy", 1}}, std::nullopt,
      {{today, "poadlegacy", 5}});
    process(container, legacy_user_id, today, "legacy");
    check_profile(container, legacy_user_id, {{today, "legacy", 2}}, std::nullopt,
      {{today, "poadlegacy", 5}});

    process(container, empty_user_id, today, "");
    if (AdServer::Commons::sync_wait(get_profile(container, empty_user_id)).in())
    {
      throw std::runtime_error("Empty URL created a profile");
    }

    process(container, user_id, today, "https://b.example/");
    process(container, user_id, today - Generics::Time::ONE_DAY, "https://z.example/");
    process(container, user_id, today - Generics::Time::ONE_DAY, "https://a.example/");
    process(container, user_id, today, "https://b.example/");
    process(container, user_id, today - Generics::Time::ONE_DAY * 30, "https://edge.example/");
    process(container, user_id, today - Generics::Time::ONE_DAY * 31, "https://old.example/");
    process(container, user_id, today, "");
    process(container, user_id, today, {"keyword2", "", "keyword1", "keyword2"});

    check_profile(
      container,
      user_id,
      {
        {today - Generics::Time::ONE_DAY, "https://z.example/", 1},
        {today, "https://b.example/", 2},
        {today, "keyword1", 1},
        {today, "keyword2", 2}
      });

    check_profile(
      container,
      user_id,
      {
        {today - Generics::Time::ONE_DAY, "https://z.example/", 1}
      },
      static_cast<std::uint32_t>((today - Generics::Time::ONE_DAY).tv_sec));

    const AdServer::Commons::UserId period_user_id =
      AdServer::Commons::UserId::create_random_based();
    const Generics::Time period = Generics::Time::ONE_DAY * USER_NAVIGATION_PERIOD_DAYS;
    const Generics::Time current_period_start(today.tv_sec / period.tv_sec * period.tv_sec);

    process(
      container,
      period_user_id,
      current_period_start - Generics::Time::ONE_DAY * 21,
      std::vector<std::string_view>{"a", "b"});
    process(container, period_user_id, current_period_start - Generics::Time::ONE_DAY * 11, "c");
    process(container, period_user_id, current_period_start - Generics::Time::ONE_DAY, "d");
    process(container, period_user_id, current_period_start, "e");

    check_profile(
      container,
      period_user_id,
      {
        {current_period_start - Generics::Time::ONE_DAY * 21, "b", 1},
        {current_period_start - Generics::Time::ONE_DAY * 11, "c", 1},
        {current_period_start - Generics::Time::ONE_DAY, "d", 1},
        {current_period_start, "e", 1}
      });

    check_profile(
      container,
      period_user_id,
      {
        {current_period_start - Generics::Time::ONE_DAY * 21, "b", 1},
        {current_period_start - Generics::Time::ONE_DAY * 11, "c", 1},
        {current_period_start - Generics::Time::ONE_DAY, "d", 1}
      },
      static_cast<std::uint32_t>((current_period_start - Generics::Time::ONE_DAY).tv_sec));

    if (container->profile_size() == 0)
    {
      throw std::runtime_error("Profile map size is zero");
    }

    container->deactivate_object();
    container->wait_object();
    container.reset();

    if (legacy_profile_exists(chunk_folders, legacy_user_id))
    {
      throw std::runtime_error("Legacy profile was not removed after migration");
    }

    std::filesystem::create_directories(root / "Unowned" / "Chunk_0_2");
    chunk_folders.clear();
    AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
      chunk_folders,
      (root / "Unowned").c_str(),
      "Chunk");
    container = new UserNavigationContainer(
      logger,
      2,
      chunk_folders,
      "UserNavigation",
      AdServer::ProfilingCommons::LevelMapTraits(
        AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
        1024 * 1024,
        1024 * 1024,
        2 * 1024 * 1024,
        20,
        Generics::Time::ONE_DAY * 30),
      USER_NAVIGATIONS_LIMIT,
      USER_NAVIGATION_PERIOD_DAYS);
    container->activate_object();

    AdServer::Commons::UserId unowned_user_id;
    do
    {
      unowned_user_id = AdServer::Commons::UserId::create_random_based();
    }
    while (AdServer::Commons::uuid_distribution_hash(unowned_user_id) % 2 == 0);

    if (AdServer::Commons::sync_wait(get_profile(container, unowned_user_id)).in())
    {
      throw std::runtime_error("Unowned chunk returned a profile");
    }

    container->deactivate_object();
    container->wait_object();
    container.reset();
    std::filesystem::remove_all(root);
    std::cout << "UserNavigationContainerTest: OK" << std::endl;
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::filesystem::remove_all(root);
    std::cerr << "UserNavigationContainerTest: " << ex.what() << std::endl;
  }

  return 1;
}
