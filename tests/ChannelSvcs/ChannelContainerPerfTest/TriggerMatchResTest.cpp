#include <cassert>
#include <iostream>

#include <ChannelSvcs/ChannelServer/ChannelChunk.hpp>

int main()
{
  namespace CS = AdServer::ChannelSvcs;
  CS::TriggerMatchRes result;
  CS::TriggerMatchRes other;
  assert(result.get_allocator().arena() != other.get_allocator().arena());

  for (unsigned int pass = 0; pass < 3; ++pass)
  {
    assert(result.empty());
    for (const auto count : result.count_channels)
    {
      assert(count == 0);
    }

    // Exceed the inline arena to exercise release and reuse with nested sets.
    for (unsigned int id = 1; id <= 1000; ++id)
    {
      auto& item = result[id];
      assert(item.flags == 0 && item.weight == 0);
      item.flags = CS::TriggerMatchItem::TMI_UID;
      item.weight = id;
      for (std::size_t type = 0; type < CS::CT_MAX; ++type)
      {
        auto& ids = item.trigger_ids[type];
        assert(ids.get_allocator().arena() == result.get_allocator().arena());
        for (unsigned int trigger = 0; trigger < 32; ++trigger)
        {
          ids.insert(id * 100 + 31 - trigger);
          ids.insert(id * 100 + 31 - trigger);
          ++result.count_channels[type];
        }
      }
    }

    assert(result.size() == 1000);
    for (unsigned int id = 1; id <= 1000; ++id)
    {
      const auto& item = result[id];
      assert(item.flags == CS::TriggerMatchItem::TMI_UID && item.weight == id);
      for (const auto& ids : item.trigger_ids)
      {
        assert(ids.size() == 32);
        unsigned int trigger = 0;
        for (const auto trigger_id : ids)
        {
          assert(trigger_id == id * 100 + trigger++);
        }
      }
    }

    result.clear();
  }

  result[7].trigger_ids[CS::CT_URL].insert(17);
  other[7].trigger_ids[CS::CT_URL].insert(27);
  result.clear();
  assert(*other[7].trigger_ids[CS::CT_URL].begin() == 27);
  std::cout << "TriggerMatchResTest passed\n";
}
