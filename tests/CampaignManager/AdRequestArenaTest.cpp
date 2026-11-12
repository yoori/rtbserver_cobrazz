#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <CampaignSvcs/CampaignManager/CampaignManagerCore.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManagerLogAdapter.hpp>

namespace
{
  using Request = AdServer::CampaignSvcs::CampaignManagerCore::CommonAdRequest;
  using StringMember = Generics::MonoString Request::*;

  constexpr StringMember STRING_MEMBERS[] = {
    &Request::creative_instantiate_type, &Request::external_user_id, &Request::source_id,
    &Request::full_referer, &Request::referer, &Request::security_token,
    &Request::pub_impr_track_url,
    &Request::pub_param, &Request::preclick_url, &Request::click_prefix_url, &Request::original_url,
    &Request::signed_user_id, &Request::peer_ip, &Request::user_agent, &Request::cohort,
    &Request::ext_track_params, &Request::passback_type, &Request::passback_url};

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  bool contains(const std::array<char, 65536>& buffer, const void* pointer)
  {
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    const auto begin = reinterpret_cast<std::uintptr_t>(buffer.data());
    return address >= begin && address < begin + buffer.size();
  }

  void fill(Request& request, std::string_view text)
  {
    for (const auto member : STRING_MEMBERS)
    {
      request.*member = text;
    }

    request.colo_id = 42;
    request.test_request = true;
    request.location.emplace_back(text, text, text);
    request.coord_location.resize(2);
    request.urls.emplace_back(text);
    request.tokens.emplace_back(text, text);
    // Exercise allocator propagation during growth and default construction.
    request.location.resize(4);
    request.tokens.resize(4);
    request.urls.resize(4);
    for (std::size_t i = 1; i < 4; ++i)
    {
      request.location[i].country = text;
      request.location[i].region = text;
      request.location[i].city = text;
      request.tokens[i].name = text;
      request.tokens[i].value = text;
      request.urls[i] = text;
    }
  }

  void check(const Request& request, const std::array<char, 65536>& buffer, std::string_view text)
  {
    const auto* arena = request.user_agent.get_allocator().arena();
    require(arena != nullptr, "missing arena");
    for (const auto member : STRING_MEMBERS)
    {
      const auto& value = request.*member;
      require(value == text, "string content changed");
      require(value.get_allocator().arena() == arena, "wrong string allocator");
      require(contains(buffer, value.data()), "string is outside arena buffer");
    }

    require(request.colo_id == 42 && request.test_request, "scalar fields changed");
    require(contains(buffer, request.location.data()), "location vector is outside arena");
    require(contains(buffer, request.coord_location.data()), "coordinates are outside arena");
    require(contains(buffer, request.urls.data()), "URLs vector is outside arena");
    require(contains(buffer, request.tokens.data()), "tokens vector is outside arena");
    for (const auto& location : request.location)
    {
      for (const auto* value : {&location.country, &location.region, &location.city})
      {
        require(*value == text, "location content changed");
        require(value->get_allocator().arena() == arena, "wrong location string allocator");
        require(contains(buffer, value->data()), "location string is outside arena");
      }
    }

    for (const auto& token : request.tokens)
    {
      for (const auto* value : {&token.name, &token.value})
      {
        require(*value == text, "token content changed");
        require(value->get_allocator().arena() == arena, "wrong token string allocator");
        require(contains(buffer, value->data()), "token string is outside arena");
      }
    }

    for (const auto& url : request.urls)
    {
      require(url == text, "URL content changed");
      require(url.get_allocator().arena() == arena, "wrong URL string allocator");
      require(contains(buffer, url.data()), "URL string is outside arena");
    }
  }

  void test_arena_storage_and_moves()
  {
    std::array<char, 65536> buffer;
    auto arena = std::make_shared<Generics::MonoAllocatorArena>(buffer.data(), buffer.size());
    std::weak_ptr<Generics::MonoAllocatorArena> weak_arena = arena;
    const std::string text(192, 'x');
    {
      Request request(arena);
      arena.reset();
      fill(request, text);
      check(request, buffer, text);

      {
        std::array<char, 65536> other_buffer;
        auto other_arena = std::make_shared<Generics::MonoAllocatorArena>(
          other_buffer.data(), other_buffer.size());
        Request copy(other_arena);
        copy.location.push_back(request.location.front());
        copy.tokens.push_back(request.tokens.front());
        copy.urls.push_back(request.urls.front());
        require(copy.location.front().city == std::string_view(text), "location copy failed");
        require(
          contains(other_buffer, copy.location.front().city.data()), "location kept old arena");
        require(contains(other_buffer, copy.tokens.front().value.data()), "token kept old arena");
        require(contains(other_buffer, copy.urls.front().data()), "URL kept old arena");
      }

      const char* data = request.user_agent.data();
      Request moved(std::move(request));
      check(moved, buffer, text);
      require(moved.user_agent.data() == data, "move constructor copied string data");

      auto old_arena = std::make_shared<Generics::MonoAllocatorArena>();
      std::weak_ptr<Generics::MonoAllocatorArena> weak_old_arena = old_arena;
      Request destination(std::move(old_arena));
      fill(destination, text);
      destination = std::move(moved);
      require(weak_old_arena.expired(), "move assignment retained old arena");
      check(destination, buffer, text);
      require(destination.user_agent.data() == data, "move assignment copied string data");
      destination = std::move(destination);
      check(destination, buffer, text);
      require(!weak_arena.expired(), "request released its arena too early");
    }

    require(weak_arena.expired(), "arena outlived request holders");
  }

  void test_default_and_null_arena()
  {
    const std::string text(192, 'y');
    Request default_request;
    fill(default_request, text);
    require(default_request.user_agent.get_allocator().arena() != nullptr, "no default arena");
    Request null_request(nullptr);
    fill(null_request, text);
    require(
      null_request.tokens.front().value == std::string_view(text), "null arena fallback failed");
  }
  using Core = AdServer::CampaignSvcs::CampaignManagerCore;
  using Slot = Core::AdSlotRequest;
  using Log = Core::LogAdRequest;

  constexpr Generics::MonoString Slot::* SLOT_STRINGS[] = {
    &Slot::format, &Slot::ext_tag_id, &Slot::min_ecpm_currency_code};
  constexpr Core::ArenaStringArray Slot::* SLOT_ARRAYS[] = {
    &Slot::sizes, &Slot::currency_codes, &Slot::exclude_categories, &Slot::required_categories};
  constexpr Generics::MonoString Log::* LOG_STRINGS[] = {
    &Log::search_words, &Log::page_keywords, &Log::url_keywords};
  constexpr Core::ArenaChannelTriggerMatchArray Core::TriggerMatchResultInfo::* MATCH_ARRAYS[] = {
    &Core::TriggerMatchResultInfo::url_channels, &Core::TriggerMatchResultInfo::pkw_channels,
    &Core::TriggerMatchResultInfo::skw_channels, &Core::TriggerMatchResultInfo::ukw_channels};

  template<typename Container>
  void check_storage(const Container& value, const std::array<char, 65536>& buffer,
    const Generics::MonoAllocatorArena* arena)
  {
    require(value.get_allocator().arena() == arena, "wrong container allocator");
    require(contains(buffer, value.data()), "data is outside arena buffer");
  }

  void fill(Slot& slot, std::string_view text)
  {
    for (const auto member : SLOT_STRINGS)
    {
      slot.*member = text;
    }

    for (const auto member : SLOT_ARRAYS)
    {
      auto& values = slot.*member;
      values.emplace_back(text);
      values.resize(4);
      for (auto& value : values)
      {
        value = text;
      }
    }

    slot.ad_slot_id = 123;
    slot.allowed_durations = {15, 30};
    auto& first_data = slot.native_data_tokens.emplace_back();
    first_data.name = text;
    first_data.required = true;
    auto& first_image = slot.native_image_tokens.emplace_back();
    first_image.name = text;
    first_image.required = true;
    first_image.width = 320;
    first_image.height = 200;
    slot.tokens.emplace_back(text, text);
    slot.native_data_tokens.resize(4);
    slot.native_image_tokens.resize(4);
    slot.tokens.resize(4);
    for (std::size_t i = 1; i < 4; ++i)
    {
      auto& data = slot.native_data_tokens[i];
      data.name = text;
      data.required = true;
      auto& image = slot.native_image_tokens[i];
      image.name = text;
      image.required = true;
      image.width = 320;
      image.height = 200;
      slot.tokens[i].name = text;
      slot.tokens[i].value = text;
    }
  }

  void check(const Slot& slot, const std::array<char, 65536>& buffer, std::string_view text)
  {
    const auto* arena = slot.format.get_allocator().arena();
    for (const auto member : SLOT_STRINGS)
    {
      check_storage(slot.*member, buffer, arena);
      require(slot.*member == text, "slot string changed");
    }

    for (const auto member : SLOT_ARRAYS)
    {
      const auto& values = slot.*member;
      check_storage(values, buffer, arena);
      for (const auto& value : values)
      {
        check_storage(value, buffer, arena);
        require(value == text, "slot string array changed");
      }
    }

    require(slot.ad_slot_id == 123, "slot ID changed");
    check_storage(slot.allowed_durations, buffer, arena);
    require(slot.allowed_durations[1] == 30, "allowed duration changed");
    check_storage(slot.native_data_tokens, buffer, arena);
    check_storage(slot.native_image_tokens, buffer, arena);
    check_storage(slot.tokens, buffer, arena);
    for (const auto& token : slot.native_data_tokens)
    {
      check_storage(token.name, buffer, arena);
      require(token.name == text && token.required, "native data token changed");
    }

    for (const auto& token : slot.native_image_tokens)
    {
      check_storage(token.name, buffer, arena);
      require(token.name == text && token.required, "native image token changed");
      require(token.width == 320 && token.height == 200, "native image dimensions changed");
    }

    for (const auto& token : slot.tokens)
    {
      check_storage(token.name, buffer, arena);
      check_storage(token.value, buffer, arena);
      require(token.name == text && token.value == text, "slot token changed");
    }
  }

  void fill(Log& log, std::string_view text)
  {
    for (const auto member : LOG_STRINGS)
    {
      log.*member = text;
    }

    log.search_engine_id = 42;
    for (const auto member : MATCH_ARRAYS)
    {
      (log.trigger_match_result.*member).push_back({17, 29});
    }

    log.trigger_match_result.uid_channels = {31, 47};
  }

  void check(const Log& log, const std::array<char, 65536>& buffer, std::string_view text)
  {
    const auto* arena = log.search_words.get_allocator().arena();
    for (const auto member : LOG_STRINGS)
    {
      check_storage(log.*member, buffer, arena);
      require(log.*member == text, "log string changed");
    }

    require(log.search_engine_id == 42, "search engine ID changed");
    for (const auto member : MATCH_ARRAYS)
    {
      const auto& values = log.trigger_match_result.*member;
      check_storage(values, buffer, arena);
      require(values.front().channel_trigger_id == 17, "trigger ID changed");
      require(values.front().channel_id == 29, "trigger channel ID changed");
    }

    check_storage(log.trigger_match_result.uid_channels, buffer, arena);
    require(log.trigger_match_result.uid_channels.back() == 47, "UID channel changed");
  }

  template<typename Owner>
  void test_owner_moves(Generics::MonoString Owner::* member)
  {
    std::array<char, 65536> buffer;
    auto arena = std::make_shared<Generics::MonoAllocatorArena>(buffer.data(), buffer.size());
    std::weak_ptr<Generics::MonoAllocatorArena> weak_arena = arena;
    const std::string text(192, 's');
    {
      Owner source(std::move(arena));
      fill(source, text);
      const char* data = (source.*member).data();
      Owner moved(std::move(source));
      check(moved, buffer, text);
      auto old_arena = std::make_shared<Generics::MonoAllocatorArena>();
      std::weak_ptr<Generics::MonoAllocatorArena> weak_old = old_arena;
      Owner destination(std::move(old_arena));
      fill(destination, text);
      destination = std::move(moved);
      require(weak_old.expired(), "move assignment retained old arena");
      check(destination, buffer, text);
      require((destination.*member).data() == data, "move copied string data");
      destination = std::move(destination);
      check(destination, buffer, text);
    }

    require(weak_arena.expired(), "owner retained arena");
    Owner default_owner;
    fill(default_owner, text);
    Owner null_owner(nullptr);
    fill(null_owner, text);
    require(null_owner.*member == std::string_view(text), "null arena fallback failed");
  }

  void test_get_request_arena()
  {
    std::array<char, 65536> buffer;
    auto arena = std::make_shared<Generics::MonoAllocatorArena>(buffer.data(), buffer.size());
    std::weak_ptr<Generics::MonoAllocatorArena> weak_arena = arena;
    const std::string text(192, 'k');
    std::shared_ptr<Slot> retained_slot;
    std::unique_ptr<Log> retained_log;
    {
      Core::GetAdRequest request(std::move(arena));
      request.common_info = std::make_shared<Request>(request.arena());
      request.ad_slots.emplace_back(std::make_shared<Slot>(request.arena()));
      fill(*request.common_info, text);
      fill(*request.ad_slots.front(), text);
      fill(request.log_request, text);
      auto& first = request.ccg_keywords.emplace_back();
      first.click_url = text;
      first.original_keyword = text;
      first.ccg_keyword_id = 17;
      first.ccg_id = 29;
      first.channel_id = 31;
      first.max_cpc = AdServer::CampaignSvcs::RevenueDecimal("1.25");
      first.ctr = AdServer::CampaignSvcs::RevenueDecimal("0.5");
      request.ccg_keywords.resize(4);
      for (std::size_t i = 1; i < request.ccg_keywords.size(); ++i)
      {
        request.ccg_keywords[i].click_url = text;
        request.ccg_keywords[i].original_keyword = text;
      }

      Core::GetAdRequest moved(std::move(request));
      check(*moved.common_info, buffer, text);
      check(*moved.ad_slots.front(), buffer, text);
      check(moved.log_request, buffer, text);
      check_storage(moved.ad_slots, buffer, moved.resource());
      check_storage(moved.ccg_keywords, buffer, moved.resource());
      require(moved.log_request.search_words.get_allocator().arena() == moved.resource(),
        "log uses a separate arena");
      for (const auto& keyword : moved.ccg_keywords)
      {
        check_storage(keyword.click_url, buffer, moved.resource());
        check_storage(keyword.original_keyword, buffer, moved.resource());
        require(keyword.click_url == std::string_view(text), "keyword URL changed");
        require(keyword.original_keyword == std::string_view(text), "keyword text changed");
      }

      std::array<char, 65536> copy_buffer;
      auto copy_arena = std::make_shared<Generics::MonoAllocatorArena>(
        copy_buffer.data(), copy_buffer.size());
      Core::GetAdRequest copy(copy_arena);
      Slot copy_slot(copy_arena);
      copy.ccg_keywords.push_back(moved.ccg_keywords.front());
      copy_slot.native_data_tokens.push_back(moved.ad_slots.front()->native_data_tokens.front());
      copy_slot.native_image_tokens.push_back(moved.ad_slots.front()->native_image_tokens.front());
      const auto& keyword = copy.ccg_keywords.front();
      check_storage(keyword.click_url, copy_buffer, copy_arena.get());
      check_storage(keyword.original_keyword, copy_buffer, copy_arena.get());
      require(keyword.ccg_keyword_id == 17 && keyword.ccg_id == 29, "keyword IDs changed");
      require(keyword.channel_id == 31, "keyword channel changed");
      require(keyword.max_cpc == AdServer::CampaignSvcs::RevenueDecimal("1.25"), "CPC changed");
      require(keyword.ctr == AdServer::CampaignSvcs::RevenueDecimal("0.5"), "CTR changed");
      check_storage(copy_slot.native_data_tokens.front().name, copy_buffer, copy_arena.get());
      check_storage(copy_slot.native_image_tokens.front().name, copy_buffer, copy_arena.get());
      require(copy_slot.native_data_tokens.front().required, "native data requirement changed");
      const auto& image = copy_slot.native_image_tokens.front();
      require(image.required && image.width == 320 && image.height == 200, "native image changed");
      retained_slot = moved.ad_slots.front();
      retained_log = std::make_unique<Log>(std::move(moved.log_request));
    }

    require(!weak_arena.expired(), "request released retained fields' arena");
    check(*retained_slot, buffer, text);
    retained_slot.reset();
    check(*retained_log, buffer, text);
    retained_log.reset();
    require(weak_arena.expired(), "request arena leaked");
  }

  void test_logging_keeps_source_arena_read_only()
  {
    using namespace AdServer::CampaignSvcs;
    std::array<char, 65536> buffer;
    auto arena = std::make_shared<Generics::MonoAllocatorArena>(buffer.data(), buffer.size());
    std::weak_ptr<Generics::MonoAllocatorArena> weak_arena = arena;
    const std::string text(192, 'l');
    CampaignManagerLogger::RequestInfo result;
    result.urls.resize(4);
    {
      Core::GetAdRequest request(std::move(arena));
      Request common(request.arena());
      Core::ContextAdRequest context(request.arena());
      Core::AdSlotContext slot_context;
      fill(common, text);
      context.client = text;
      context.client_version = text;
      context.full_platform = text;
      context.web_browser = text;
      context.ip_hash = text;
      context.additional_info = text;
      fill(request.log_request, text);
      Generics::MonoAllocator<char> probe(request.resource());
      const char* before = probe.allocate(1);
      CampaignManagerLogAdapter::fill_request_info(
        result, nullptr, nullptr, common, context, &request.log_request, &request.channels,
        true, false, slot_context);
      const char* after = probe.allocate(1);
      require(after == before + 1, "logging allocated memory in the source arena");
    }

    require(weak_arena.expired(), "logging retained source arena");
    buffer.fill('!');
    for (const auto* value : {&result.external_id, &result.referer, &result.country_code,
      &result.client_app, &result.client_app_version, &result.full_platform,
      &result.web_browser, &result.ip_hash, &result.additional_info})
    {
      require(value->get_allocator().arena() == result.arena().get(),
        "logged string borrowed source allocator");
      require(*value == std::string_view(text), "logged string borrowed source storage");
    }

    require(result.urls.size() == 4, "logged URLs changed");
    for (const auto& url : result.urls)
    {
      require(url.get_allocator().arena() == result.arena().get(),
        "logged URL borrowed source allocator");
      require(url == std::string_view(text), "logged URL borrowed source storage");
    }

    require(result.user_agent->str() == text, "logged user agent changed");
    require(result.search_words->str() == text, "logged search words changed");
    require(result.page_keywords->str() == text, "logged page keywords changed");
    require(result.url_keywords->str() == text, "logged URL keywords changed");
  }

}

int main()
{
  try
  {
    test_arena_storage_and_moves();
    test_default_and_null_arena();
    test_owner_moves(&Slot::format);
    test_owner_moves(&Log::search_words);
    test_get_request_arena();
    test_logging_keeps_source_arena_read_only();
    std::cout << "AdRequestArenaTest: PASS\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
