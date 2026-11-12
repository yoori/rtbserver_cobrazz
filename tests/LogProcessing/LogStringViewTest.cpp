#include <array>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string_view>
#include <typeinfo>

#include <Generics/MonoAllocator.hpp>
#include <LogCommons/Request.hpp>
#include <LogCommons/RequestBasicChannels.hpp>
#include <LogCommons/SearchEngineStat.hpp>
#include <LogCommons/TagRequest.hpp>

#ifndef LOG_STRING_VIEW_COUNT_ALLOCATIONS
#define LOG_STRING_VIEW_COUNT_ALLOCATIONS 0
#endif

namespace
{
  constexpr std::size_t TEXT_SIZE = 8191;
  thread_local bool count_allocations = false;
  thread_local std::size_t string_allocations = 0;
}

#if LOG_STRING_VIEW_COUNT_ALLOCATIONS
void* operator new(std::size_t size)
{
  if (void* memory = std::malloc(size ? size : 1))
  {
    if (count_allocations && size == TEXT_SIZE + 1)
    {
      ++string_allocations;
    }

    return memory;
  }

  throw std::bad_alloc();
}

void operator delete(void* memory) noexcept
{
  std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
  std::free(memory);
}

#endif

namespace
{
  using namespace AdServer::LogProcessing;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  template<typename Builder>
  auto build_from_arena(Builder builder, std::size_t expected_allocations)
  {
    std::array<char, 16384> buffer{};
    Generics::MonoAllocatorArena arena(buffer.data(), buffer.size());
    Generics::MonoString text(TEXT_SIZE, 'x', Generics::MonoAllocator<char>(&arena));
    text[17] = '\0';
    string_allocations = 0;
    count_allocations = true;
    try
    {
      auto result = builder(std::string_view(text));
      count_allocations = false;
      if (LOG_STRING_VIEW_COUNT_ALLOCATIONS && string_allocations != expected_allocations)
      {
        std::cerr << typeid(result).name() << ": " << string_allocations
          << " allocations, expected " << expected_allocations << '\n';
        throw std::runtime_error("unexpected string allocation count");
      }
      text.assign(TEXT_SIZE, '!');
      return result;
    }
    catch (...)
    {
      count_allocations = false;
      throw;
    }
  }

  void check_text(std::string_view text)
  {
    require(text.size() == TEXT_SIZE, "stored string length changed");
    for (std::size_t i = 0; i < text.size(); ++i)
    {
      require(text[i] == (i == 17 ? '\0' : 'x'), "record borrowed source storage or lost NUL");
    }
  }

  RequestData make_request(std::string_view text)
  {
    Generics::MonoAllocatorArena arena;
    Generics::MonoList<FixedNumber> ctrs{Generics::MonoAllocator<FixedNumber>(&arena)};
    ctrs.emplace_back("0.125");
    ctrs.emplace_back("0.5");
    return RequestData(
      {}, // time
      {}, // isp_time
      {}, // pub_time
      {}, // adv_time
      {}, // request_id
      {}, // global_request_id
      {}, // user_id
      {}, // household_id
      {}, // test_request
      {}, // colo_id
      {}, // site_id
      {}, // tag_id
      text, // ext_tag_id
      {}, // publisher_account_id
      "ru", // country_code
      text, // ip_address
      {}, // adv_account_id
      {}, // advertiser_id
      {}, // cc_id
      {}, // cmp_id
      {}, // ccg_id
      {}, // delivery_threshold
      {}, // has_custom_actions
      {}, // currency_exchange_id
      {}, // user_props
      {}, // adv_revenue
      {}, // pub_revenue
      {}, // isp_revenue
      {}, // adv_comm_revenue
      {}, // adv_payable_comm_amount
      {}, // pub_comm_revenue
      {}, // channel_list
      {}, // history_channel_list
      text, // channel_expression
      {}, // cmp_channel_list
      {}, // ccg_keyword_id
      {}, // keyword_id
      {}, // keyword_page_match
      {}, // keyword_search_match
      {}, // num_shown
      {}, // position
      {}, // enabled_notice
      {}, // enabled_impression_tracking
      {}, // enabled_action_tracking
      {}, // disable_fraud_detection
      {}, // walled_garden
      {}, // ccg_type
      {}, // user_status
      {}, // lost_auction_ccgs
      {}, // geo_channels
      {}, // device_channel_id
      text, // tag_size
      {}, // size_id
      {}, // hid_profile
      {}, // tag_visibility
      {}, // tag_top_offset
      {}, // tag_left_offset
      {}, // ctr_reset_id
      {}, // campaign_freq
      text, // referer
      {}, // adv_currency_rate
      {}, // pub_currency_rate
      {}, // pub_commission
      {}, // isp_currency_rate
      {}, // isp_revenue_share
      {}, // ecpm
      {}, // floor_cost
      text, // ctr_algorithm_id
      {}, // ctr
      {}, // full_referer_hash
      {}, // auction_type
      text, // conv_rate_algorithm_id
      {}, // conv_rate
      {}, // tag_predicted_viewability
      FixedNumberList(ctrs.begin(), ctrs.end()), // model_ctrs
      {}, // self_service_commission
      {}, // adv_commission
      {}, // pub_cost_coef
      {}, // at_flags
      text, // additional_info
      "", // page_keywords
      {}); // expected_post_actions
  }
  void test_records()
  {
    auto request = build_from_arena(make_request, 8);
    check_text(request.ip_address());
    check_text(request.referer());
    check_text(request.additional_info());
    check_text(request.ext_tag_id());
    check_text(request.channel_expression());
    check_text(request.tag_size());
    check_text(request.ctr_algorithm_id());
    check_text(request.conv_rate_algorithm_id());
    require(request.model_ctrs().size() == 2 &&
      request.model_ctrs().front() == FixedNumber("0.125") &&
      request.model_ctrs().back() == FixedNumber("0.5"), "record borrowed model CTR storage");

    auto basic = build_from_arena([](std::string_view text)
    {
      return RequestBasicChannelsInnerData('P', {}, {}, {}, {}, text, text, "");
    }, 2);
    check_text(basic.external_id());
    check_text(basic.referer());

    auto tag = build_from_arena([](std::string_view text)
    {
      return TagRequestData({}, {}, false, 1, 2, 3, {}, text, text, {}, 'I', "ru", {}, {}, {}, {});
    }, 2);
    check_text(tag.referer());
    check_text(tag.ext_tag_id());

    auto key = build_from_arena([](std::string_view text)
    {
      return SearchEngineStatInnerKey(1, text);
    }, 1);
    check_text(key.host_name());
    require(key == SearchEngineStatInnerKey(1, key.host_name()), "host hash key changed");

    auto property = build_from_arena([](std::string_view text)
    {
      return UserProperty("name", text);
    }, 1);
    check_text(property.second);

    auto props = build_from_arena([](std::string_view text)
    {
      return RequestBasicChannelsInnerData::AdRequestProps(
        {}, text, 0, {}, {}, {}, {}, AdServer::CampaignSvcs::AT_MAX_ECPM);
    }, 1);
    check_text(props.country_code());
  }

  void test_empty_and_existing_string_inputs()
  {
    const std::string_view empty;
    StringIoWrapperOptional optional(empty);
    require(!optional.present(), "empty string became present");
    EmptyHolder<std::string> holder(empty);
    require(holder.get().empty(), "empty holder changed");
    SpacesString literal("literal");
    const std::string original("owned string");
    SpacesString owned(original);
    require(literal == "literal" && owned == original, "existing string constructor changed");
    auto request = make_request(empty);
    require(request.ip_address().empty() && request.referer().empty() &&
      request.additional_info().empty(), "empty request strings changed");
  }
}

int main()
{
  try
  {
    test_records();
    test_empty_and_existing_string_inputs();
    std::cout << "LogStringViewTest: PASS\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << "LogStringViewTest: " << error.what() << '\n';
    return 1;
  }
}
