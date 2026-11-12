#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <CampaignSvcs/CampaignManager/CampaignManagerLogAdapter.hpp>

namespace
{
  using namespace AdServer::CampaignSvcs;
  using Logger = CampaignManagerLogger;
  using Selection = Logger::AdRequestSelectionInfo;
  using Ad = Logger::AdSelectionInfo;
  using Core = CampaignManagerCore;
  using Buffer = std::array<char, 65536>;

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
    const auto begin = reinterpret_cast<std::uintptr_t>(buffer.data());
    return address >= begin && address < begin + buffer.size();
  }

  template<typename Container>
  void check_storage(const Container& value, const Generics::MonoAllocatorArena* arena,
    const Buffer* buffer)
  {
    require(value.get_allocator().arena() == arena, "wrong allocator");
    require(!value.empty(), "lost container contents");
    if (buffer)
    {
      require(contains(*buffer, std::addressof(*value.begin())), "data outside arena buffer");
    }
  }

  void fill(Ad& ad, std::string_view text)
  {
    ad.ad_selected = true;
    ad.cc_id = 17;
    ad.adv_revenue.impression = RevenueDecimal("0.125");
    ad.expression = text;
    ad.ctr_algorithm_id = text;
    ad.conv_rate_algorithm_id = text;
    ad.channels = {17, 29};
    ad.keyword_ccg_ids = {31, 41};
    ad.model_ctrs = {RevenueDecimal("0.25"), RevenueDecimal("0.5")};
    Logger::CMPChannel channel{};
    channel.channel_id = 19;
    channel.channel_rate_id = 23;
    channel.imp_revenue = RevenueDecimal::ZERO;
    channel.imp_sys_revenue = RevenueDecimal::ZERO;
    channel.adv_imp_revenue = RevenueDecimal::ZERO;
    channel.click_revenue = RevenueDecimal::ZERO;
    channel.click_sys_revenue = RevenueDecimal::ZERO;
    channel.adv_click_revenue = RevenueDecimal::ZERO;
    ad.cmp_channels.push_back(channel);
  }

  void fill(Selection& selection, std::string_view text)
  {
    selection.tag_id = 43;
    selection.walled_garden = true;
    selection.floor_cost = RevenueDecimal("0.75");
    selection.ext_tag_id = text;
    selection.tag_size = text;
    selection.tag_sizes.emplace(text);
    selection.lost_auction_ccgs.emplace_back(Generics::Time(1), 47);
    selection.lost_auction_creatives.emplace_back(Generics::Time(2), 53);
    selection.ad_selection_info_list.resize(2);
    for (auto& ad : selection.ad_selection_info_list)
    {
      fill(ad, text);
    }
  }

  void check(const Ad& ad, std::string_view text, const Buffer* buffer = nullptr)
  {
    require(ad.ad_selected && ad.cc_id == 17, "lost ad scalar fields");
    require(ad.adv_revenue.impression == RevenueDecimal("0.125"), "lost revenue");
    for (const auto* value : {&ad.expression, &ad.ctr_algorithm_id, &ad.conv_rate_algorithm_id})
    {
      check_storage(*value, ad.arena().get(), buffer);
      require(*value == text, "lost ad string");
    }

    check_storage(ad.channels, ad.arena().get(), buffer);
    check_storage(ad.cmp_channels, ad.arena().get(), buffer);
    check_storage(ad.keyword_ccg_ids, ad.arena().get(), buffer);
    check_storage(ad.model_ctrs, ad.arena().get(), buffer);
    require(ad.channels.front() == 17 && ad.channels.back() == 29, "lost channels");
    require(ad.cmp_channels.front().channel_rate_id == 23, "lost CMP channels");
    require(ad.keyword_ccg_ids.count(31) && ad.keyword_ccg_ids.count(41), "lost keyword CCGs");
    require(ad.model_ctrs.front() == RevenueDecimal("0.25") &&
      ad.model_ctrs.back() == RevenueDecimal("0.5"), "lost model CTRs");
  }

  void check(const Selection& selection, std::string_view text, const Buffer* buffer = nullptr)
  {
    const auto* arena = selection.arena().get();
    require(selection.tag_id == 43 && selection.walled_garden, "lost selection scalar fields");
    require(selection.floor_cost == RevenueDecimal("0.75"), "lost floor cost");
    check_storage(selection.ext_tag_id, arena, buffer);
    check_storage(selection.tag_size, arena, buffer);
    require(selection.ext_tag_id == text && selection.tag_size == text, "lost selection strings");
    check_storage(selection.tag_sizes, arena, buffer);
    for (const auto& size : selection.tag_sizes)
    {
      check_storage(size, arena, buffer);
      require(size == text, "lost nested tag size string");
    }

    check_storage(selection.lost_auction_ccgs, arena, buffer);
    check_storage(selection.lost_auction_creatives, arena, buffer);
    require(selection.lost_auction_ccgs.front().id == 47, "lost auction CCGs");
    require(selection.lost_auction_creatives.front().id == 53, "lost auction creatives");
    require(selection.ad_selection_info_list.size() == 2, "lost ads");
    check_storage(selection.ad_selection_info_list, arena, buffer);
    for (const auto& ad : selection.ad_selection_info_list)
    {
      require(ad.arena() == selection.arena(), "ad has a separate arena");
      check(ad, text, buffer);
    }
  }

  void test_copy_move_and_lifetime()
  {
    const std::string text(256, 'x');
    Buffer source_buffer{};
    Buffer copy_buffer{};
    auto source = std::make_unique<Selection>(std::make_shared<Generics::MonoAllocatorArena>(
      source_buffer.data(), source_buffer.size()));
    fill(*source, text);
    check(*source, text, &source_buffer);
    const std::weak_ptr<Generics::MonoAllocatorArena> source_arena = source->arena();
    Selection copy(*source);
    require(copy.arena() != source->arena(), "copy shares mutable arena");
    Selection explicit_copy(*source, std::make_shared<Generics::MonoAllocatorArena>(
      copy_buffer.data(), copy_buffer.size()));
    Selection assigned;
    fill(assigned, text);
    const std::weak_ptr<Generics::MonoAllocatorArena> old_arena = assigned.arena();
    assigned = *source;
    require(old_arena.expired(), "copy assignment retained old arena");
    source.reset();
    require(source_arena.expired(), "copy retained source arena");
    source_buffer.fill('!');
    check(copy, text);
    check(assigned, text);
    check(explicit_copy, text, &copy_buffer);

    const auto* pointer = copy.ext_tag_id.data();
    const auto* node = &copy.ad_selection_info_list.front();
    Selection moved(std::move(copy));
    require(moved.ext_tag_id.data() == pointer, "move copied a string");
    require(&moved.ad_selection_info_list.front() == node, "move copied list nodes");
    const std::weak_ptr<Generics::MonoAllocatorArena> assigned_arena = assigned.arena();
    assigned = std::move(moved);
    require(assigned_arena.expired(), "move assignment retained old arena");
    check(assigned, text);
    assigned = assigned;
    assigned = std::move(assigned);
    check(assigned, text);

    auto& child = assigned.ad_selection_info_list.front();
    child = explicit_copy.ad_selection_info_list.front();
    require(child.arena() == assigned.arena(), "element assignment changed the destination arena");
    child = std::move(explicit_copy.ad_selection_info_list.front());
    require(child.arena() == assigned.arena(), "element move assignment changed destination arena");
    check(child, text);
  }

  void test_insert_and_retained_ad()
  {
    const std::string text(256, 'a');
    std::unique_ptr<Ad> retained;
    std::weak_ptr<Generics::MonoAllocatorArena> arena;
    {
      Selection selection;
      Ad source;
      fill(source, text);
      selection.ad_selection_info_list.push_back(source);
      selection.ad_selection_info_list.push_back(std::move(source));
      for (const auto& ad : selection.ad_selection_info_list)
      {
        require(ad.arena() == selection.arena(), "insertion did not propagate allocator");
        check(ad, text);
      }

      arena = selection.arena();
      retained = std::make_unique<Ad>(std::move(selection.ad_selection_info_list.front()));
    }

    require(!arena.expired(), "moved out ad lost its arena");
    check(*retained, text);
    retained.reset();
    require(arena.expired(), "retained ad leaked arena");
  }

  void test_adapter_does_not_allocate_in_source()
  {
    using namespace AdInstances;
    const std::string text(256, 's');
    auto config = std::make_shared<CampaignConfig>();
    config->currency_exchange_id = 1;
    Currency_var currency(new Currency());
    currency->rate = RevenueDecimal(1);
    Account_var account(new AccountDef());
    account->account_id = 1;
    account->currency = currency;
    Site_var site(new Site());
    site->site_id = 2;
    site->account = account;
    Tag_var tag(new Tag());
    tag->tag_id = 3;
    tag->site = site;
    Colocation_var colocation(new Colocation());
    colocation->account = account;
    colocation->colo_rate_id = 4;
    Selection result;
    {
      auto arena = std::make_shared<Generics::MonoAllocatorArena>(65536);
      Core::CommonAdRequest common(arena);
      Core::ContextAdRequest context(arena);
      Core::AdSlotRequest slot(arena);
      Core::AdSlotContext slot_context;
      Core::AdSlotMinCpm min_cpm;
      AdSelectionResult selected;
      slot.ext_tag_id = text;
      Generics::MonoAllocator<char> probe(arena.get());
      const char* before = probe.allocate(1);
      CampaignManagerLogAdapter::fill_ad_request_selection_info(
        result, config.get(), colocation, common, context, nullptr, slot, tag, selected,
        slot_context, min_cpm, tag->sizes, false);
      require(probe.allocate(1) == before + 1, "adapter allocated in source arena");
    }

    require(result.ext_tag_id == std::string_view(text), "adapter lost external tag ID");
    require(result.ext_tag_id.get_allocator().arena() == result.arena().get(),
      "adapter copied source allocator");
    require(result.ad_selection_info_list.size() == 1, "adapter lost unselected ad");
    require(result.ad_selection_info_list.front().arena() == result.arena(),
      "adapter created a separate arena for ad");
  }
}

int main()
{
  try
  {
    test_copy_move_and_lifetime();
    test_insert_and_retained_ad();
    test_adapter_does_not_allocate_in_source();
    std::cout << "LoggerSelectionArenaTest: PASS\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
