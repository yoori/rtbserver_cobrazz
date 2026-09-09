#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
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
#include <RequestInfoSvcs/ExpressionMatcher/UserNavigationContainer.hpp>
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
    const std::vector<ExpectedNavigation>& expected)
  {
    const AdServer::RequestInfoSvcs::UserNavigationProfileReader reader(
      profile->membuf().data(),
      profile->membuf().size());

    std::size_t expected_days = 0;
    std::optional<Generics::Time> previous_date;
    for (const auto& navigation : expected)
    {
      if (!previous_date.has_value() || *previous_date != navigation.date)
      {
        ++expected_days;
        previous_date = navigation.date;
      }
    }

    if (reader.days().size() != expected_days)
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
  }

  void
  check_profile(
    UserNavigationContainer* container,
    const AdServer::Commons::UserId& user_id,
    const std::vector<ExpectedNavigation>& expected,
    std::optional<std::uint32_t> date = std::nullopt)
  {
    const Generics::ConstSmartMemBuf_var profile = AdServer::Commons::sync_wait(
      get_profile(container, user_id, date));
    if (!profile.in())
    {
      throw std::runtime_error("Profile is absent");
    }

    check_profile_data(profile, expected);
  }

  void
  check_v1_adapter()
  {
    AdServer::RequestInfoSvcs_v1::UserNavigationProfileWriter old_profile;
    old_profile.version() = 1;
    for (const auto& navigation_info : std::vector<ExpectedNavigation>{
      {Generics::Time(10), "a", 1},
      {Generics::Time(10), "b", 2},
      {Generics::Time(20), "c", 3}})
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
      });
  }
}

int
main()
{
  constexpr std::size_t USER_NAVIGATIONS_LIMIT = 4;

  const std::filesystem::path root = std::filesystem::temp_directory_path() /
    ("UserNavigationContainerTest-" + std::to_string(::getpid()));

  try
  {
    check_v1_adapter();

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "Chunk_0_1");

    AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_folders;
    AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
      chunk_folders,
      root.c_str(),
      "Chunk");

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
        USER_NAVIGATIONS_LIMIT);
    container->activate_object();

    const AdServer::Commons::UserId user_id =
      AdServer::Commons::UserId::create_random_based();
    const AdServer::Commons::UserId empty_user_id =
      AdServer::Commons::UserId::create_random_based();
    const Generics::Time today = Algs::round_to_day(Generics::Time::get_time_of_day());

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

    if (container->profile_size() == 0)
    {
      throw std::runtime_error("Profile map size is zero");
    }

    container->deactivate_object();
    container->wait_object();
    container.reset();

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
      USER_NAVIGATIONS_LIMIT);
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
