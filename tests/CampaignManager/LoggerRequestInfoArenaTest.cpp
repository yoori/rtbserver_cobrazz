#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <CampaignSvcs/CampaignManager/CampaignManagerLogger.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelIndex.hpp>

namespace
{
  using Logger = AdServer::CampaignSvcs::CampaignManagerLogger;
  using Request = Logger::RequestInfo;
  using Buffer = std::array<char, 65536>;
  using StringMember = Generics::MonoString Request::*;

  constexpr StringMember STRINGS[] = {
    &Request::client_app, &Request::client_app_version, &Request::full_platform,
    &Request::web_browser, &Request::search_engine_host, &Request::country_code,
    &Request::external_id, &Request::referer, &Request::ip_hash, &Request::additional_info};

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  bool contains(const Buffer& buffer, const void* pointer)
  {
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    const auto start = reinterpret_cast<std::uintptr_t>(buffer.data());
    return address >= start && address < start + buffer.size();
  }

  void fill(Request& request, std::string_view text)
  {
    for (const auto member : STRINGS)
    {
      request.*member = text;
    }

    request.colo_id = 42;
    request.random = 123;
    request.is_ad_request = true;
    request.user_status = AdServer::CampaignSvcs::US_OPTIN;
    request.urls.resize(3);
    for (auto& url : request.urls)
    {
      url = text;
    }

    for (auto* channels : {&request.platforms, &request.triggered_channels.channels,
      &request.triggered_channels.url_channels, &request.triggered_channels.page_channels,
      &request.triggered_channels.search_channels, &request.triggered_channels.url_keyword_channels,
      &request.triggered_channels.uid_channels})
    {
      channels->insert(17);
      channels->insert(29);
    }

    request.platform_channels.insert(17);
    request.geo_channels = {17, 29};
    request.history_channels = {17, 29};
    request.hid_history_channels = {17, 29};
    request.hit_keywords = {17, 29};
    for (auto* triggers : {&request.page_triggers, &request.search_triggers,
      &request.url_triggers, &request.url_keyword_triggers})
    {
      triggers->emplace_back(17, 29);
    }

    for (auto* holder : {&request.search_words, &request.user_agent,
      &request.page_keywords, &request.url_keywords})
    {
      *holder = new AdServer::Commons::StringHolder(text);
    }
  }

  template<typename Container>
  void check_container(const Container& values, const Request& request, const Buffer* buffer)
  {
    require(values.get_allocator().arena() == request.arena().get(), "wrong container arena");
    require(!values.empty(), "lost container contents");
    if (buffer)
    {
      require(contains(*buffer, std::addressof(*values.begin())), "element outside arena buffer");
    }
  }

  void check(const Request& request, std::string_view text, const Buffer* buffer = nullptr)
  {
    require(request.arena() != nullptr, "missing arena");
    for (const auto member : STRINGS)
    {
      const auto& value = request.*member;
      require(value == text, "lost string contents");
      require(value.get_allocator().arena() == request.arena().get(), "wrong string arena");
      if (buffer)
      {
        require(contains(*buffer, value.data()), "string outside arena buffer");
      }
    }

    require(request.colo_id == 42 && request.random == 123, "lost scalar fields");
    require(request.is_ad_request, "lost boolean field");
    require(request.user_status == AdServer::CampaignSvcs::US_OPTIN, "lost user status");
    require(request.urls.size() == 3, "lost URLs");
    check_container(request.urls, request, buffer);
    for (const auto& url : request.urls)
    {
      require(url == text, "lost URL contents");
      require(url.get_allocator().arena() == request.arena().get(), "wrong nested URL arena");
      if (buffer)
      {
        require(contains(*buffer, url.data()), "URL string outside arena buffer");
      }
    }

    for (const auto* channels : {&request.platforms, &request.triggered_channels.channels,
      &request.triggered_channels.url_channels, &request.triggered_channels.page_channels,
      &request.triggered_channels.search_channels, &request.triggered_channels.url_keyword_channels,
      &request.triggered_channels.uid_channels})
    {
      check_container(*channels, request, buffer);
      require(channels->count(17) && channels->count(29), "lost triggered channels");
    }

    check_container(request.platform_channels, request, buffer);
    check_container(request.geo_channels, request, buffer);
    check_container(request.history_channels, request, buffer);
    check_container(request.hid_history_channels, request, buffer);
    check_container(request.hit_keywords, request, buffer);
    for (const auto* triggers : {&request.page_triggers, &request.search_triggers,
      &request.url_triggers, &request.url_keyword_triggers})
    {
      check_container(*triggers, request, buffer);
      require(triggers->size() == 1, "lost triggers");
    }

    for (const auto* holder : {&request.search_words, &request.user_agent,
      &request.page_keywords, &request.url_keywords})
    {
      require(holder->in() && (*holder)->str() == text, "lost StringHolder contents");
    }
  }

  void test_copy_move_and_lifetime()
  {
    const std::string text(256, 'x');
    Buffer source_buffer{};
    Buffer copy_buffer{};
    auto source = std::make_unique<Request>(
      std::make_shared<Generics::MonoAllocatorArena>(source_buffer.data(), source_buffer.size()));
    fill(*source, text);
    check(*source, text, &source_buffer);
    const std::weak_ptr<Generics::MonoAllocatorArena> source_arena = source->arena();

    Request copy(*source);
    require(copy.arena() != source->arena(), "copy shares mutable arena");
    Request explicit_copy(*source,
      std::make_shared<Generics::MonoAllocatorArena>(copy_buffer.data(), copy_buffer.size()));
    check(explicit_copy, text, &copy_buffer);
    Request assigned;
    fill(assigned, "old contents long enough to allocate outside SSO");
    const std::weak_ptr<Generics::MonoAllocatorArena> old_arena = assigned.arena();
    assigned = *source;
    require(old_arena.expired(), "copy assignment retained old arena");
    require(assigned.arena() != source->arena(), "assignment shares mutable arena");
    Logger::AnonymousRequestInfo anonymous(*source);
    require(anonymous.arena() != source->arena(), "anonymous copy shares mutable arena");

    source.reset();
    require(source_arena.expired(), "copy retained source arena");
    source_buffer.fill('!');
    check(copy, text);
    check(assigned, text);
    check(explicit_copy, text, &copy_buffer);
    require(std::string_view(anonymous.client_app) == text,
      "anonymous copy references destroyed source");
    require(anonymous.platforms.count(17), "anonymous copy lost platforms");
    require(anonymous.platform_channels.count(17), "anonymous copy lost platform channels");

    const auto* moved_arena = copy.arena().get();
    Request moved(std::move(copy));
    require(moved.arena().get() == moved_arena, "move allocated another arena");
    check(moved, text);
    const std::weak_ptr<Generics::MonoAllocatorArena> assigned_arena = assigned.arena();
    assigned = std::move(moved);
    require(assigned_arena.expired(), "move assignment retained old arena");
    require(assigned.arena().get() == moved_arena, "move assignment copied arena");
    check(assigned, text);
    assigned = assigned;
    assigned = std::move(assigned);
    check(assigned, text);
  }

  void test_platform_matching()
  {
    using namespace AdServer::CampaignSvcs;
    ExpressionChannelHolderMap channels;
    for (const auto id : {17UL, 29UL})
    {
      ChannelParams params(id);
      params.status = 'A';
      SimpleChannel_var channel(new SimpleChannel(params));
      channels[id] = new ExpressionChannelHolder(channel);
    }

    ExpressionChannelIndex_var index(new ExpressionChannelIndex());
    index->index(channels);
    Request request;
    request.platforms.insert(17);
    ChannelIdSet expected;
    index->match(expected, request.platforms);
    index->match(request.platform_channels, request.platforms);
    require(!expected.empty(), "platform matching produced no channels");
    require(std::equal(expected.begin(), expected.end(),
      request.platform_channels.begin(), request.platform_channels.end()),
      "platform matching changed with arena allocator");
    check_container(request.platform_channels, request, nullptr);
  }

  void test_string_holders_outlive_arena()
  {
    const std::string text(256, 's');
    std::array<AdServer::Commons::StringHolder_var, 4> holders;
    std::weak_ptr<Generics::MonoAllocatorArena> arena;
    {
      Request request;
      fill(request, text);
      arena = request.arena();
      holders = {request.search_words, request.user_agent,
        request.page_keywords, request.url_keywords};
    }

    require(arena.expired(), "escaping strings retained request arena");
    for (const auto& holder : holders)
    {
      require(holder->str() == text, "escaping string references destroyed arena");
    }
  }
}

int main()
{
  try
  {
    test_copy_move_and_lifetime();
    test_string_holders_outlive_arena();
    test_platform_matching();
    std::cout << "LoggerRequestInfoArenaTest: OK\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << "LoggerRequestInfoArenaTest: " << error.what() << '\n';
    return 1;
  }
}
