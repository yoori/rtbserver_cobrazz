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

    const AdServer::RequestInfoSvcs::UserNavigationProfileReader reader(
      profile->membuf().data(),
      profile->membuf().size());
    if (reader.navigations().size() != expected.size())
    {
      throw std::runtime_error("Unexpected navigation count");
    }

    auto navigation = reader.navigations().begin();
    for (const auto& expected_navigation : expected)
    {
      if ((*navigation).date() != expected_navigation.date.tv_sec ||
        (*navigation).url() != expected_navigation.url ||
        (*navigation).count() != expected_navigation.count)
      {
        throw std::runtime_error("Unexpected navigation entry");
      }

      ++navigation;
    }
  }
}

int
main()
{
  const std::filesystem::path root = std::filesystem::temp_directory_path() /
    ("UserNavigationContainerTest-" + std::to_string(::getpid()));

  try
  {
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
          Generics::Time::ONE_DAY * 30));
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
        {today - Generics::Time::ONE_DAY * 30, "https://edge.example/", 1},
        {today - Generics::Time::ONE_DAY, "https://a.example/", 1},
        {today - Generics::Time::ONE_DAY, "https://z.example/", 1},
        {today, "https://b.example/", 2},
        {today, "keyword1", 1},
        {today, "keyword2", 2}
      });

    check_profile(
      container,
      user_id,
      {
        {today - Generics::Time::ONE_DAY * 30, "https://edge.example/", 1},
        {today - Generics::Time::ONE_DAY, "https://a.example/", 1},
        {today - Generics::Time::ONE_DAY, "https://z.example/", 1}
      },
      static_cast<std::uint32_t>((today - Generics::Time::ONE_DAY).tv_sec));

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
