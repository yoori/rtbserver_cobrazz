#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <Commons/PlainMonoAllocatorTypes.hpp>
#include "CustomWriterTest.hpp"

namespace
{
  using MonoTypes = AdServer::Commons::PlainMonoAllocatorTypes;

  struct CountingViewTypes: MonoTypes
  {
    static std::size_t allocated_bytes;
    static char* allocate_string(Context& context, std::size_t size);
  };

  std::size_t CountingViewTypes::allocated_bytes = 0;

  char* CountingViewTypes::allocate_string(Context& context, std::size_t size)
  {
    allocated_bytes += size;
    return MonoTypes::allocate_string(context, size);
  }

  void check(bool value, const char* message)
  {
    if (!value)
    {
      throw std::runtime_error(message);
    }
  }

  template<typename Writer>
  std::vector<unsigned char> save(const Writer& writer)
  {
    std::vector<unsigned char> result(writer.size());
    writer.save(result.data(), result.size());
    return result;
  }

  template<typename Item>
  void fill_item(Item& item)
  {
    item.flag() = 'x';
    item.signed_value() = -17;
    item.value() = 42;
    item.big() = 0xfedcba9876543210ULL;
    item.text().assign(200, 'z');
    const unsigned char bytes[] = {0, 1, 0, 255};
    item.blob().assign(bytes, sizeof(bytes));
  }

  template<typename Writer>
  void fill(Writer& writer)
  {
    writer.version() = 7;
    fill_item(writer.embedded());
    fill_item(writer.hidden());
    writer.fixed().value() = 43;
    writer.entries().resize(2);
    fill_item(writer.entries().front());
    fill_item(writer.entries().back());
    writer.old_entries().resize(2);
    fill_item(writer.old_entries().front());
    fill_item(writer.old_entries().back());
    writer.numbers().push_back(13);
    writer.numbers().push_back(27);
    writer.old_numbers().push_back(0x12345678abcdef00ULL);
    writer.old_numbers().push_back(0xffffffffffffffffULL);
  }

  template<typename Types>
  void verify(const std::vector<unsigned char>& expected)
  {
    using Writer = PlainCustomTest::FullCustomWriter<Types>;
    static_assert(std::is_nothrow_move_constructible_v<Writer>);
    typename Types::Context context;
    Writer writer(context);
    fill(writer);
    check(save(writer) == expected, "Custom filling differs from legacy Writer");

    PlainCustomTest::ProfileCustomWriter<Types> partial(context, expected.data(), expected.size());
    check(save(partial) == expected, "Hidden descriptor fields were lost");
    partial.init(expected.data(), expected.size());
    check(save(partial) == expected, "Repeated initialization appended list elements");
    partial.entries().resize(1);
    partial.entries().emplace(partial.entries().begin());
    fill_item(partial.entries().front());
    partial.old_entries().resize(1);
    fill_item(partial.old_entries().emplace_back());
    check(save(partial) == expected, "Context-aware vector/list growth differs");

    auto copy = writer;
    auto moved = std::move(copy);
    check(save(moved) == expected, "Move construction lost data");
    typename Types::Context other_context;
    Writer assigned(other_context);
    assigned = writer;
    check(&assigned.context() == &context, "Copy assignment lost context");
    check(save(assigned) == expected, "Copy assignment lost data");
    Writer swapped(other_context);
    swapped.swap(assigned);
    check(&swapped.context() == &context, "Swap lost context");
    check(save(swapped) == expected, "Swap lost data");
    assigned = std::move(swapped);
    fill_item(assigned.entries().emplace_back());
    assigned.entries().pop_back();
    check(save(assigned) == expected, "Move assignment/growth lost data");

    typename Writer::entries_Container entries(other_context);
    entries.swap(assigned.entries());
    check(&entries.context() == &context, "Container swap lost context");
    fill_item(entries.emplace_back());
    entries.pop_back();
    entries.swap(assigned.entries());
    check(save(assigned) == expected, "Container swap lost data");

    if constexpr (std::is_same_v<Types, MonoTypes>)
    {
      check(assigned.entries().get_allocator().arena() == &context, "Wrong vector arena");
      check(assigned.old_entries().get_allocator().arena() == &context, "Wrong list arena");
      check(assigned.numbers().get_allocator().arena() == &context, "Wrong scalar vector arena");
      check(assigned.old_numbers().get_allocator().arena() == &context, "Wrong scalar list arena");
      check(&assigned.entries().back().text().context() == &context,
        "Wrong string arena after moving");
      check(assigned.embedded().blob().get_allocator().arena() == &context, "Wrong buffer arena");
    }

    assigned.init_default();
    check(assigned.version() == 0 && assigned.entries().empty() && assigned.old_entries().empty(),
      "init_default did not clear the Writer");
    assigned = PlainTypes::ConstBuf(expected.data(), expected.size());
    check(save(assigned) == expected, "ConstBuf assignment lost data");

    auto corrupt = expected;
    const std::uint32_t bad_offset = expected.size() + 1;
    std::memcpy(corrupt.data() + Writer::numbers_OFFSET, &bad_offset, sizeof(bad_offset));
    bool rejected = false;
    try
    {
      assigned.init(corrupt.data(), corrupt.size());
    }
    catch (const PlainTypes::CorruptedStruct&)
    {
      rejected = true;
    }
    check(rejected, "Invalid scalar array range was accepted");
  }

  void verify_lazy_string()
  {
    using String = PlainTypes::CustomString<CountingViewTypes>;
    static_assert(std::is_same_v<MonoTypes::String, std::string_view>);
    static_assert(!std::is_base_of_v<std::string_view, String>);
    CountingViewTypes::Context context;
    const std::string text(200, 'a');
    std::vector<char> buffer(1024 * 1024, 'z');
    const std::uint32_t offset = String::FIXED_SIZE;
    std::memcpy(buffer.data(), &offset, sizeof(offset));
    std::memcpy(buffer.data() + offset, text.c_str(), text.size() + 1);

    String value(context);
    value.init(buffer.data(), buffer.size());
    check(value.data() == buffer.data() + offset, "init copied the borrowed string");
    auto copy = value;
    check(copy.data() == value.data(), "Wrapper copy allocated string storage");
    const std::string owned(copy);
    check(owned == text && owned.capacity() < buffer.size(), "String conversion copied the tail");
    check(std::string_view(value) == text, "View conversion included bytes after the terminator");
    check(value.size() == text.size() && copy.size() == text.size(), "Wrong cached string length");
    check(CountingViewTypes::allocated_bytes == 0, "Reading a borrowed string allocated in arena");

    std::vector<char> output(offset + value.dyn_size_(), 'z');
    value.save_(output.data(), output.data() + offset);
    check(output.size() == offset + text.size() + 1 && output.back() == '\0',
      "Serialization included the unused buffer tail");
    check(std::memcmp(output.data() + offset, text.data(), text.size()) == 0,
      "Serialized string changed");

    copy.append("suffix");
    check(std::string(copy) == text + "suffix", "Append lost the borrowed prefix");
    check(CountingViewTypes::allocated_bytes == text.size() + 6,
      "Append allocated space for the whole source buffer");
    check(std::string(value) == text, "Append changed the original view");
    check(std::memcmp(buffer.data() + offset, text.c_str(), text.size() + 1) == 0,
      "Append changed the input profile");

    const std::array<char, 3> unterminated = {'x', 'y', 'z'};
    value.assign(unterminated.data(), unterminated.size());
    check(value.data() == unterminated.data(), "assign copied an explicit view");
    output.resize(offset + value.dyn_size_());
    value.save_(output.data(), output.data() + offset);
    check(output.back() == '\0' && std::string(output.data() + offset) == "xyz",
      "Saving an explicit view requires an input terminator");

    std::vector<char> invalid(offset + 3, 'x');
    std::memcpy(invalid.data(), &offset, sizeof(offset));
    for (unsigned int operation = 0; operation < 4; ++operation)
    {
      value.init(invalid.data(), invalid.size());
      bool rejected = false;
      try
      {
        switch (operation)
        {
          case 0: value.size(); break;
          case 1: static_cast<std::string>(value); break;
          case 2: static_cast<std::string_view>(value); break;
          case 3: value.save_(output.data(), output.data() + offset); break;
        }
      }
      catch (const PlainTypes::CorruptedStruct&)
      {
        rejected = true;
      }
      check(rejected, "Lazy read accepted a missing terminator");
    }

    value.init(buffer.data(), buffer.size());
    check(std::string(value) == text, "Repeated init retained a previous cached length");
    value.clear();
    check(value.empty() && value.dyn_size_() == 1, "Empty view has an invalid serialized size");
  }

  template<typename Types>
  void verify_nested_containers()
  {
    using Strings = PlainTypes::CustomList<Types, PlainTypes::CustomString<Types>>;
    using Rows = PlainTypes::CustomVector<Types, Strings>;
    typename Types::Context context;
    Rows rows(context);
    rows.resize(2);
    rows.front().emplace_back().assign(300, 'a');
    rows.back().emplace_back().assign(400, 'b');
    std::vector<unsigned char> buffer(Rows::FIXED_SIZE + rows.dyn_size_());
    rows.save_(buffer.data(), buffer.data() + Rows::FIXED_SIZE);
    Rows restored(context);
    restored.init(buffer.data(), buffer.size());
    check(restored.size() == 2 &&
      std::string_view(restored.front().front()) == std::string_view(rows.front().front()) &&
      std::string_view(restored.back().front()) == std::string_view(rows.back().front()),
      "Nested containers lost data");
    if constexpr (std::is_same_v<Types, MonoTypes>)
    {
      check(restored.back().get_allocator().arena() == &context, "Wrong nested list arena");
      check(&restored.back().front().context() == &context,
        "Wrong nested string arena");
    }
  }
}

int main()
{
  try
  {
    PlainCustomTest::FullWriter legacy(true);
    fill(legacy);
    const auto expected = save(legacy);
    verify<PlainTypes::DefaultWritingTypes>(expected);
    verify<MonoTypes>(expected);
    verify_lazy_string();
    verify_nested_containers<PlainTypes::DefaultWritingTypes>();
    verify_nested_containers<MonoTypes>();
    std::cout << "PlainCustomWriterTest: OK\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
