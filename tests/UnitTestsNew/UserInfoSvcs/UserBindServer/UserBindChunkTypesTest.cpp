// GTEST
#include <gtest/gtest.h>

// STD
#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>

#define private public

// THIS
#include <UserInfoSvcs/UserBindServer/UserBindChunkTypes.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindChunk.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindChunkTwoLayers.hpp>
#include <UserInfoSvcs/UserBindServer/Utils.hpp>

using namespace AdServer::UserInfoSvcs;

namespace
{

[[maybe_unused]] void remove_directory(const std::string& str_path)
{
  std::filesystem::path path(str_path);
  std::filesystem::remove_all(path);
}

} // namespace

TEST(Stream, Types)
{
  {
    std::stringstream stream;

    BoundUserInfoHolder holder;
    holder.flags = 'a';
    holder.initial_day = 1;
    holder.bad_event_count = 22;
    holder.last_bad_event_day = 330;
    holder.user_id = Generics::Uuid::create_random_based();

    stream << holder;
    BoundUserInfoHolder result;
    EXPECT_TRUE(stream.good());
    stream >> result;
    EXPECT_TRUE(stream.eof());
    EXPECT_EQ(result, holder);
  }

  {
    std::stringstream stream;

    SeenUserInfoHolder holder;
    holder.first_seen_time_offset_ = 1;
    holder.initial_day_ = 22;

    stream << holder;
    EXPECT_TRUE(stream.good());
    SeenUserInfoHolder result;
    stream >> result;
    EXPECT_TRUE(stream.eof());
    EXPECT_EQ(result, holder);
  }

  {
    const std::string text = "text";

    std::stringstream stream;
    StringDefHashAdapter adapter(text, 777);
    stream << adapter;
    EXPECT_TRUE(stream.good());

    StringDefHashAdapter result;
    stream >> result;
    EXPECT_TRUE(stream.eof());
    EXPECT_EQ(adapter, result);
  }

  {
    const std::string text = "";

    std::stringstream stream;
    StringDefHashAdapter adapter(text, 777);
    stream << adapter;
    EXPECT_TRUE(stream.good());

    StringDefHashAdapter result;
    stream >> result;
    EXPECT_TRUE(stream.eof());
    EXPECT_EQ(adapter, result);
  }

  {
    std::stringstream stream;
    HashHashAdapter adapter(777);
    stream << adapter;
    EXPECT_TRUE(stream.good());

    HashHashAdapter result;
    stream >> result;
    EXPECT_TRUE(stream.eof());
    EXPECT_EQ(adapter, result);
  }

  {
    const std::string text = "text";

    std::stringstream stream;
    ExternalIdHashAdapter adapter(text, Utils::hash(text));
    stream << adapter;
    EXPECT_TRUE(stream.good());
    ExternalIdHashAdapter result;
    stream >> result;
    EXPECT_TRUE(stream.eof());
    EXPECT_EQ(adapter, result);
  }

  {
    const std::string text = "";

    std::stringstream stream;
    ExternalIdHashAdapter adapter(text, Utils::hash(text));
    stream << adapter;
    EXPECT_TRUE(stream.good());
    ExternalIdHashAdapter result;
    stream >> result;
    EXPECT_TRUE(stream.eof());
    EXPECT_EQ(adapter, result);
  }

  {
    const std::string text = "text";

    std::stringstream stream;
    ExternalIdHashAdapter key(text, Utils::hash(text));

    SeenUserInfoHolder value;
    value.first_seen_time_offset_ = 1;
    value.initial_day_ = 22;

    stream << key << ' ' << value;
    EXPECT_TRUE(stream.good());

    ExternalIdHashAdapter key_result;
    SeenUserInfoHolder value_result;
    stream >> key_result >> value_result;
    EXPECT_TRUE(stream.eof());
    EXPECT_EQ(key_result, key);
    EXPECT_EQ(value_result, value);
  }
}

TEST(UserBindChunkTypes, Time)
{
  const auto day1 = get_current_day_from_2000();
  const auto day2 = get_current_day_from_2000();
  EXPECT_EQ(day1, day2);
}

namespace
{

class ContainerTest : public testing::Test
{
protected:
  void SetUp() override
  {
    holder1.flags = '1';
    holder1.initial_day = 1;
    holder1.bad_event_count = 11;
    holder1.last_bad_event_day = 111;
    holder1.user_id = Generics::Uuid::create_random_based();

    holder2.flags = '2';
    holder2.initial_day = 2;
    holder2.bad_event_count = 22;
    holder2.last_bad_event_day = 222;
    holder2.user_id = Generics::Uuid::create_random_based();

    holder3.flags = '3';
    holder3.initial_day = 3;
    holder3.bad_event_count = 33;
    holder3.last_bad_event_day = 333;
    holder3.user_id = Generics::Uuid::create_random_based();

    logger = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::EMERGENCY));
  }

  BoundUserInfoHolder holder1;
  StringDefHashAdapter key1 = {"key1", 1000};
  BoundUserInfoHolder holder2;
  StringDefHashAdapter key2 = {"key2", 1001};
  BoundUserInfoHolder holder3;
  StringDefHashAdapter key3 = {"key3", 1002};

  Logging::Logger_var logger;
  const std::string db_path = "/tmp/rocksdb_test";
};

} // namespace

TEST_F(ContainerTest, MemoryContainer)
{
  MemoryContainer<StringDefHashAdapter, BoundUserInfoHolder> container;
  BoundUserInfoHolder result;

  container.set(key1, holder1);
  container.set(key2, holder2);
  container.set(key3, holder3);

  EXPECT_TRUE(container.get(result, key1));
  EXPECT_EQ(result, holder1);

  EXPECT_TRUE(container.get(result, key2));
  EXPECT_EQ(result, holder2);

  EXPECT_TRUE(container.get(result, key3));
  EXPECT_EQ(result, holder3);

  EXPECT_TRUE(container.erase(key2));
  EXPECT_FALSE(container.erase(key2));
  EXPECT_FALSE(container.get(result, key2));
}

TEST_F(ContainerTest, RocksdbContainer)
{
  remove_directory(db_path);

  for (int i = 1; i <= 2; ++i)
  {
    auto column_families = create_rocksdb(
      logger.in(),
      db_path,
      {"test1", "test2"},
      5,
      rocksdb::CompactionStyle::kCompactionStyleLevel,
      100,
      100);

    auto rocksdb_params1 = std::make_shared<RocksdbParams>(
      logger.in(),
      column_families[0],
      std::make_shared<rocksdb::ReadOptions>(),
      std::make_shared<rocksdb::WriteOptions>());
    RocksdbContainer<StringDefHashAdapter, BoundUserInfoHolder> container1(rocksdb_params1);

    auto rocksdb_params2 = std::make_shared<RocksdbParams>(
      logger.in(),
      column_families[1],
      std::make_shared<rocksdb::ReadOptions>(),
      std::make_shared<rocksdb::WriteOptions>());
    RocksdbContainer<StringDefHashAdapter, BoundUserInfoHolder> container2(rocksdb_params2);

    BoundUserInfoHolder result;

    container1.set(key1, holder1);
    container1.set(key2, holder2);
    container2.set(key3, holder3);

    EXPECT_TRUE(container1.get(result, key1));
    EXPECT_EQ(result, holder1);

    EXPECT_TRUE(container1.get(result, key2));
    EXPECT_EQ(result, holder2);

    EXPECT_FALSE(container1.get(result, key3));

    EXPECT_TRUE(container2.get(result, key3));
    EXPECT_EQ(result, holder3);

    EXPECT_TRUE(container2.erase(key2));
    EXPECT_FALSE(container2.get(result, key2));

    if (i == 1)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

namespace
{

class SaveLoadTest : public testing::Test
{
protected:
  void SetUp() override
  {
    std::filesystem::remove(file_seen_path);
    std::filesystem::remove(file_bound_path);

    bound_holder1.flags = '1';
    bound_holder1.initial_day = 1;
    bound_holder1.bad_event_count = 11;
    bound_holder1.last_bad_event_day = 111;
    bound_holder1.user_id = Generics::Uuid::create_random_based();

    seen_holder1.first_seen_time_offset_ = 1111;
    seen_holder1.initial_day_ = 1112;

    key1 = "key1";
    external_id1 = ExternalIdHashAdapter(key1, Utils::hash(key1));

    bound_holder2.flags = '2';
    bound_holder2.initial_day = 2;
    bound_holder2.bad_event_count = 22;
    bound_holder2.last_bad_event_day = 222;
    bound_holder2.user_id = Generics::Uuid::create_random_based();

    seen_holder2.first_seen_time_offset_ = 2222;
    seen_holder2.initial_day_ = 2223;

    key2 = "key2";
    external_id2 = ExternalIdHashAdapter(key2, Utils::hash(key2));

    bound_holder3.flags = '3';
    bound_holder3.initial_day = 3;
    bound_holder3.bad_event_count = 33;
    bound_holder3.last_bad_event_day = 333;
    bound_holder3.user_id = Generics::Uuid::create_random_based();

    seen_holder3.first_seen_time_offset_ = 3333;
    seen_holder3.initial_day_ = 3334;

    key3 = "key3";
    external_id3 = ExternalIdHashAdapter(key3, Utils::hash(key3));

    memory_seen_container = std::make_shared<
      MemoryContainer<HashHashAdapter, SeenUserInfoHolder>>(2, 2);
    memory_bound_container = std::make_shared<
      MemoryContainer<ExternalIdHashAdapter, BoundUserInfoHolder>>(2, 2);
  }

  BoundUserInfoHolder bound_holder1;
  SeenUserInfoHolder seen_holder1;
  std::string key1;
  ExternalIdHashAdapter external_id1;

  BoundUserInfoHolder bound_holder2;
  SeenUserInfoHolder seen_holder2;
  std::string key2;
  ExternalIdHashAdapter external_id2;

  BoundUserInfoHolder bound_holder3;
  SeenUserInfoHolder seen_holder3;
  std::string key3;
  ExternalIdHashAdapter external_id3;
  Logging::Logger_var logger;

  const std::string file_seen_path = "/tmp/seen_path_test";
  const std::string file_bound_path = "/tmp/bount_path_test";

  MemoryContainerPtr<HashHashAdapter, SeenUserInfoHolder> memory_seen_container;
  MemoryContainerPtr<ExternalIdHashAdapter, BoundUserInfoHolder> memory_bound_container;
};

} // namespace

TEST_F(SaveLoadTest, Test1)
{
  {
    std::ofstream file_seen_stream(file_seen_path, std::ios::out | std::ios::trunc);
    std::ofstream file_bound_stream(file_bound_path, std::ios::out | std::ios::trunc);

    memory_bound_container->set(external_id1, bound_holder1);
    memory_bound_container->set(external_id2, bound_holder2);
    memory_bound_container->set(external_id3, bound_holder3);

    memory_seen_container->set(external_id1, seen_holder1);
    memory_seen_container->set(external_id2, seen_holder2);
    memory_seen_container->set(external_id3, seen_holder3);

    GenericChunkFileHeader header(
      ChunkFileHeader::name(),
      ChunkFileHeader::current_version());

    file_seen_stream << header;
    file_bound_stream << header;

    memory_bound_container->save(
      file_bound_stream,
      SourceFilterWrapper<FilterAlwaysTrue>{FilterAlwaysTrue{}, ""});
    memory_seen_container->save(
      file_seen_stream,
      FilterAlwaysTrue{});
  }

  for (int i = 1; i <= 10; ++i)
  {
    memory_seen_container = std::make_shared<
      MemoryContainer<HashHashAdapter, SeenUserInfoHolder>>(
        2,
        2);
    memory_bound_container = std::make_shared<
      MemoryContainer<ExternalIdHashAdapter, BoundUserInfoHolder>>(
        2,
        2);

    Loader<HashHashAdapter, SeenUserInfoHolder> seen_loader(
      file_seen_path,
      *memory_seen_container);
    Loader<ExternalIdHashAdapter, BoundUserInfoHolder> bound_loader(
      file_bound_path,
      *memory_bound_container);

    BoundUserInfoHolder bound_result;
    SeenUserInfoHolder seen_result;

    EXPECT_FALSE(memory_seen_container->get(seen_result, external_id1));
    EXPECT_FALSE(memory_bound_container->get(bound_result, external_id1));

    seen_loader.load();
    bound_loader.load();

    EXPECT_TRUE(memory_seen_container->get(seen_result, external_id1));
    EXPECT_EQ(seen_result, seen_holder1);
    EXPECT_TRUE(memory_seen_container->get(seen_result, external_id2));
    EXPECT_EQ(seen_result, seen_holder2);
    EXPECT_TRUE(memory_seen_container->get(seen_result, external_id3));
    EXPECT_EQ(seen_result, seen_holder3);

    EXPECT_TRUE(memory_bound_container->get(bound_result, external_id1));
    EXPECT_EQ(bound_result, bound_holder1);
    EXPECT_TRUE(memory_bound_container->get(bound_result, external_id2));
    EXPECT_EQ(bound_result, bound_holder2);
    EXPECT_TRUE(memory_bound_container->get(bound_result, external_id3));
    EXPECT_EQ(bound_result, bound_holder3);
  }
}

TEST_F(SaveLoadTest, Test2)
{
  {
    std::ofstream file_seen_stream(file_seen_path, std::ios::out | std::ios::trunc);

    GenericChunkFileHeader header(
      ChunkFileHeader::name(),
      ChunkFileHeader::current_version());
    file_seen_stream << header;

    memory_seen_container->save(file_seen_stream, FilterAlwaysTrue{});
  }

  {
    memory_seen_container = std::make_shared<
      MemoryContainer<HashHashAdapter, SeenUserInfoHolder>>(
        2,
        2);
    Loader<HashHashAdapter, SeenUserInfoHolder> seen_loader(
      file_seen_path,
      *memory_seen_container);
    seen_loader.load();
    SeenUserInfoHolder seen_result;
    EXPECT_FALSE(memory_seen_container->get(seen_result, external_id1));
  }
}

TEST_F(SaveLoadTest, Test3)
{
  std::filesystem::remove(file_seen_path);
  std::filesystem::remove(file_bound_path);
  memory_seen_container = std::make_shared<
    MemoryContainer<HashHashAdapter, SeenUserInfoHolder>>(2, 2);
  Loader<HashHashAdapter, SeenUserInfoHolder> seen_loader(
    file_seen_path,
    *memory_seen_container);
  EXPECT_NO_THROW(seen_loader.load());
}

TEST(StringDefHashAdapter, Test)
{
  const std::string text1 = "Text1";
  const std::string text2 = "Text2";

  {
    StringDefHashAdapter adapter1(text1, 111);
    StringDefHashAdapter adapter2(text2, 222);
    adapter2 = adapter1;
    EXPECT_EQ(adapter2, StringDefHashAdapter(text1, 111));
    EXPECT_EQ(adapter1, StringDefHashAdapter(text1, 111));
  }

  {
    StringDefHashAdapter adapter1(text1, 111);
    StringDefHashAdapter adapter2(text2, 222);
    adapter2 = std::move(adapter1);
    EXPECT_EQ(adapter2, StringDefHashAdapter(text1, 111));
    EXPECT_EQ(adapter1, StringDefHashAdapter{});
  }
}

namespace
{

class FilterDay1 final
{
public:
  FilterDay1() = default;

  ~FilterDay1() = default;

  bool operator()(const SeenUserInfoHolder& value) const noexcept
  {
    return value.init_day() != 1;
  }

  bool operator()(
    const BoundUserInfoHolder& value,
    const std::string&) const noexcept
  {
    return value.init_day() != 1;
  }
};

class BoundContainersTest : public testing::Test
{
protected:
  void SetUp() override
  {
    bound_holder11.flags = 'a';
    bound_holder11.initial_day = 1;
    bound_holder11.bad_event_count = 11;
    bound_holder11.last_bad_event_day = 110;
    bound_holder11.user_id = Generics::Uuid::create_random_based();

    bound_holder12.flags = 'b';
    bound_holder12.initial_day = 1;
    bound_holder12.bad_event_count = 12;
    bound_holder12.last_bad_event_day = 120;
    bound_holder12.user_id = Generics::Uuid::create_random_based();

    bound_holder21.flags = 'c';
    bound_holder21.initial_day = 2;
    bound_holder21.bad_event_count = 21;
    bound_holder21.last_bad_event_day = 210;
    bound_holder21.user_id = Generics::Uuid::create_random_based();

    bound_holder22.flags = 'd';
    bound_holder22.initial_day = 2;
    bound_holder22.bad_event_count = 22;
    bound_holder22.last_bad_event_day = 220;
    bound_holder22.user_id = Generics::Uuid::create_random_based();

    logger = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::EMERGENCY));

    remove_directory(file_path);
    remove_directory(rocksdb_path);

    auto column_families = create_rocksdb(
      logger.in(),
      rocksdb_path,
      {"BoundUserInfoHolder"},
      5,
      rocksdb::CompactionStyle::kCompactionStyleLevel,
      100,
      100);

    rocksdb_params = std::make_shared<RocksdbParams>(
      logger.in(),
      column_families[0],
      std::make_shared<rocksdb::ReadOptions>(),
      std::make_shared<rocksdb::WriteOptions>());

    bound_containers = std::make_shared<
      BoundContainers<
        StringDefHashAdapter,
        ExternalIdHashAdapter,
        BoundUserInfoHolder>>(
          1,
          1,
          rocksdb_params);
  }

  BoundUserInfoHolder bound_holder11;
  std::string key_prefix_str1 = "key_prefix_str1";
  StringDefHashAdapter key_prefix1{key_prefix_str1, Utils::hash(key_prefix_str1)};
  std::string key_suffix_str11 = "key_suffix_str11";
  ExternalIdHashAdapter key_suffix11{key_suffix_str11, Utils::hash(key_suffix_str11)};

  BoundUserInfoHolder bound_holder12;
  std::string key_suffix_str12 = "key_suffix_str12";
  ExternalIdHashAdapter key_suffix12{key_suffix_str12, Utils::hash(key_suffix_str12)};

  BoundUserInfoHolder bound_holder21;
  std::string key_prefix_str2 = "key_prefix_str2";
  StringDefHashAdapter key_prefix2{key_prefix_str2, Utils::hash(key_prefix_str2)};
  std::string key_suffix_str21 = "key_suffix_str21";
  ExternalIdHashAdapter key_suffix21{key_suffix_str21, Utils::hash(key_suffix_str21)};

  BoundUserInfoHolder bound_holder22;
  std::string key_suffix_str22 = "key_suffix_str22";
  ExternalIdHashAdapter key_suffix22{key_suffix_str22, Utils::hash(key_suffix_str22)};

  Logging::Logger_var logger;
  RocksdbParamsPtr rocksdb_params;
  BoundContainersPtr<
    StringDefHashAdapter,
    ExternalIdHashAdapter,
    BoundUserInfoHolder> bound_containers;

  const std::string rocksdb_path = "/tmp/rocksdb_test";
  const std::string file_path = "/tmp/bound_conteiners_test";
};

} // namespace

TEST_F(BoundContainersTest, Test)
{
  bound_containers->set(key_prefix1, key_suffix11, bound_holder11);
  bound_containers->set(key_prefix1, key_suffix12, bound_holder12);
  bound_containers->set(key_prefix2, key_suffix21, bound_holder21);
  bound_containers->set(key_prefix2, key_suffix22, bound_holder22);

  BoundUserInfoHolder result;
  {
    auto container = bound_containers->get(result, key_prefix1, key_suffix11);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, bound_holder11);
    EXPECT_TRUE(container->get(result, key_suffix11));
    EXPECT_EQ(result, bound_holder11);

    auto rocksdb_container = std::dynamic_pointer_cast<
      RocksdbContainer<ExternalIdHashAdapter, BoundUserInfoHolder>>(container);
    EXPECT_FALSE(rocksdb_container);
  }
  {
    auto container = bound_containers->get(result, key_prefix1, key_suffix12);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, bound_holder12);
    EXPECT_TRUE(container->get(result, key_suffix12));
    EXPECT_EQ(result, bound_holder12);
  }
  {
    auto container = bound_containers->get(result, key_prefix2, key_suffix21);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, bound_holder21);
    EXPECT_TRUE(container->get(result, key_suffix21));
    EXPECT_EQ(result, bound_holder21);
  }
  {
    auto container = bound_containers->get(result, key_prefix2, key_suffix22);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, bound_holder22);
    EXPECT_TRUE(container->get(result, key_suffix22));
    EXPECT_EQ(result, bound_holder22);
  }

  std::stringstream stream;
  bound_containers->save(stream, FilterDay1{});

  {
    auto container = bound_containers->get(result, key_prefix1, key_suffix11);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, bound_holder11);

    auto memory_container = std::dynamic_pointer_cast<
      MemoryContainer<ExternalIdHashAdapter, BoundUserInfoHolder>>(container);
    EXPECT_TRUE(memory_container);

    EXPECT_EQ(result, bound_holder11);
    EXPECT_TRUE(container->get(result, key_suffix11));
    EXPECT_EQ(result, bound_holder11);
  }
  {
    auto container = bound_containers->get(result, key_prefix1, key_suffix12);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, bound_holder12);

    auto memory_container = std::dynamic_pointer_cast<
      MemoryContainer<ExternalIdHashAdapter, BoundUserInfoHolder>>(container);
    EXPECT_TRUE(memory_container);

    EXPECT_EQ(result, bound_holder12);
    EXPECT_TRUE(container->get(result, key_suffix12));
    EXPECT_EQ(result, bound_holder12);
  }

  {
    auto container = bound_containers->get(result, key_prefix2, key_suffix21);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, bound_holder21);

    auto rocksdb_container = std::dynamic_pointer_cast<
      RocksdbContainer<ExternalIdHashAdapter, BoundUserInfoHolder>>(container);
    EXPECT_FALSE(rocksdb_container);

    EXPECT_EQ(result, bound_holder21);
    EXPECT_TRUE(container->get(result, key_suffix21));
    EXPECT_EQ(result, bound_holder21);
  }
  {
    auto container = bound_containers->get(result, key_prefix2, key_suffix22);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, bound_holder22);

    auto rocksdb_container = std::dynamic_pointer_cast<
      RocksdbContainer<ExternalIdHashAdapter, BoundUserInfoHolder>>(container);
    EXPECT_FALSE(rocksdb_container);

    EXPECT_EQ(result, bound_holder22);
    EXPECT_TRUE(container->get(result, key_suffix22));
    EXPECT_EQ(result, bound_holder22);
  }
}

TEST_F(BoundContainersTest, Erase)
{
  BoundUserInfoHolder result;
  {
    bound_containers->set(key_prefix1, key_suffix11, bound_holder11);
    EXPECT_TRUE(bound_containers->get(result, key_prefix1, key_suffix11));
    EXPECT_EQ(result, bound_holder11);
    bound_containers->erase(key_prefix1, key_suffix11);
    EXPECT_FALSE(bound_containers->get(result, key_prefix1, key_suffix11));
  }
  {
    bound_containers->set(key_prefix1, key_suffix11, bound_holder11);
    std::stringstream stream;
    bound_containers->save(stream, FilterDay1{});
    bound_containers->erase(key_prefix1, key_suffix11);
    EXPECT_FALSE(bound_containers->get(result, key_prefix1, key_suffix11));
  }
}

TEST_F(BoundContainersTest, Load)
{
  GenericChunkFileHeader header(
    ChunkFileHeader::name(),
    ChunkFileHeader::current_version());

  {
    auto bound_containers = std::make_shared<
      BoundContainers<
        StringDefHashAdapter,
        ExternalIdHashAdapter,
        BoundUserInfoHolder>>(
          1,
          1,
          rocksdb_params);

    bound_containers->set(key_prefix1, key_suffix11, bound_holder11);
    bound_containers->set(key_prefix1, key_suffix12, bound_holder12);
    bound_containers->set(key_prefix2, key_suffix21, bound_holder21);
    bound_containers->set(key_prefix2, key_suffix22, bound_holder22);

    std::ofstream file_bound_stream(file_path, std::ios::out | std::ios::trunc);
    file_bound_stream << header;

    bound_containers->save(file_bound_stream, FilterAlwaysTrue{});
  }

  {
    auto bound_containers = std::make_shared<
      BoundContainers<
        StringDefHashAdapter,
        ExternalIdHashAdapter,
        BoundUserInfoHolder>>(
          1,
          1,
          rocksdb_params);
    Loader<std::pair<StringDefHashAdapter, ExternalIdHashAdapter>, BoundUserInfoHolder> loader(
      file_path,
      *bound_containers);
    loader.load();

    BoundUserInfoHolder result;
    EXPECT_TRUE(bound_containers->get(result, key_prefix1, key_suffix11));
    EXPECT_EQ(result, bound_holder11);
    EXPECT_TRUE(bound_containers->get(result, key_prefix1, key_suffix12));
    EXPECT_EQ(result, bound_holder12);
    EXPECT_TRUE(bound_containers->get(result, key_prefix2, key_suffix21));
    EXPECT_EQ(result, bound_holder21);
    EXPECT_TRUE(bound_containers->get(result, key_prefix2, key_suffix22));
    EXPECT_EQ(result, bound_holder22);

    bound_containers->erase(key_prefix1, key_suffix11);
    bound_containers->erase(key_prefix1, key_suffix12);

    std::ofstream file_bound_stream(file_path, std::ios::out | std::ios::trunc);
    file_bound_stream << header;
    bound_containers->save(file_bound_stream, FilterAlwaysTrue{});
  }

  {
    auto bound_containers = std::make_shared<
      BoundContainers<
        StringDefHashAdapter,
        ExternalIdHashAdapter,
        BoundUserInfoHolder>>(
          1,
          1,
          rocksdb_params);
    Loader<std::pair<StringDefHashAdapter, ExternalIdHashAdapter>, BoundUserInfoHolder> loader(
      file_path,
      *bound_containers);
    loader.load();

    BoundUserInfoHolder result;
    EXPECT_TRUE(bound_containers->get(result, key_prefix2, key_suffix21));
    EXPECT_EQ(result, bound_holder21);
    EXPECT_TRUE(bound_containers->get(result, key_prefix2, key_suffix22));
    EXPECT_EQ(result, bound_holder22);
    EXPECT_FALSE(bound_containers->get(result, key_prefix1, key_suffix11));
    EXPECT_FALSE(bound_containers->get(result, key_prefix1, key_suffix12));
  }
}

class SeenContainersTest : public testing::Test
{
protected:
  void SetUp() override
  {
    seen_holder1.initial_day_ = 1;
    seen_holder1.first_seen_time_offset_ = 10;

    seen_holder2.initial_day_ = 2;
    seen_holder2.first_seen_time_offset_ = 20;

    seen_holder3.initial_day_ = 3;
    seen_holder3.first_seen_time_offset_ = 30;

    logger = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::EMERGENCY));

    remove_directory(file_path);
    remove_directory(rocksdb_path);

    auto column_families = create_rocksdb(
      logger.in(),
      rocksdb_path,
      {"SeenUserInfoHolder"},
      5,
      rocksdb::CompactionStyle::kCompactionStyleLevel,
      100,
      100);

    rocksdb_params = std::make_shared<RocksdbParams>(
      logger.in(),
      column_families[0],
      std::make_shared<rocksdb::ReadOptions>(),
      std::make_shared<rocksdb::WriteOptions>());
    seen_containers = std::make_shared<SeenContainers<HashHashAdapter, SeenUserInfoHolder>>(
      1,
      1,
      rocksdb_params);
  }

  SeenUserInfoHolder seen_holder1;
  std::string key_str1 = "key1";
  HashHashAdapter key1{Utils::hash(key_str1)};

  SeenUserInfoHolder seen_holder2;
  std::string key_str2 = "key2";
  HashHashAdapter key2{Utils::hash(key_str2)};

  SeenUserInfoHolder seen_holder3;
  std::string key_str3 = "key3";
  HashHashAdapter key3{Utils::hash(key_str3)};

  Logging::Logger_var logger;
  RocksdbParamsPtr rocksdb_params;
  SeenContainersPtr<HashHashAdapter, SeenUserInfoHolder> seen_containers;

  const std::string rocksdb_path = "/tmp/rocksdb_test";
  const std::string file_path = "/tmp/seen_conteiners_test";
};

TEST_F(SeenContainersTest, Test)
{
  seen_containers->set(key1, seen_holder1);
  seen_containers->set(key2, seen_holder2);
  seen_containers->set(key3, seen_holder3);

SeenUserInfoHolder result;
  {
    auto container = seen_containers->get(result, key1);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, seen_holder1);
    EXPECT_TRUE(container->get(result, key1));
    EXPECT_EQ(result, seen_holder1);
    auto rocksdb_container = std::dynamic_pointer_cast<
      RocksdbContainer<HashHashAdapter, SeenUserInfoHolder>>(container);
    EXPECT_FALSE(rocksdb_container);
  }
  {
    auto container = seen_containers->get(result, key2);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, seen_holder2);
    EXPECT_TRUE(container->get(result, key2));
    EXPECT_EQ(result, seen_holder2);
  }
  {
    auto container = seen_containers->get(result, key3);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, seen_holder3);
    EXPECT_TRUE(container->get(result, key3));
    EXPECT_EQ(result, seen_holder3);
  }

  std::stringstream stream;
  seen_containers->save(stream, FilterDay1{});

  {
    auto container = seen_containers->get(result, key1);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, seen_holder1);
    EXPECT_TRUE(container->get(result, key1));
    EXPECT_EQ(result, seen_holder1);
    auto memory_container = std::dynamic_pointer_cast<
      MemoryContainer<HashHashAdapter, SeenUserInfoHolder>>(container);
    EXPECT_TRUE(memory_container);
  }
  {
    auto container = seen_containers->get(result, key2);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, seen_holder2);
    EXPECT_TRUE(container->get(result, key2));
    EXPECT_EQ(result, seen_holder2);
    auto rocksdb_container = std::dynamic_pointer_cast<
      RocksdbContainer<HashHashAdapter, SeenUserInfoHolder>>(container);
    EXPECT_FALSE(rocksdb_container);
  }
  {
    auto container = seen_containers->get(result, key3);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, seen_holder3);
    EXPECT_TRUE(container->get(result, key3));
    EXPECT_EQ(result, seen_holder3);
  }
  {
    seen_containers->erase(key1);
    auto container = seen_containers->get(result, key1);
    EXPECT_FALSE(container);
  }
}

TEST_F(SeenContainersTest, Load)
{
  GenericChunkFileHeader header(
    ChunkFileHeader::name(),
    ChunkFileHeader::current_version());

  {
    std::ofstream file_seen_stream(file_path, std::ios::out | std::ios::trunc);
    file_seen_stream << header;

    seen_containers->set(key1, seen_holder1);
    seen_containers->set(key2, seen_holder2);
    seen_containers->set(key3, seen_holder3);
    seen_containers->save(file_seen_stream, FilterDay1{});
  }

  {
    auto load_seen_containers = std::make_shared<SeenContainers<HashHashAdapter, SeenUserInfoHolder>>(
      1,
      1,
      rocksdb_params);

    Loader<HashHashAdapter, SeenUserInfoHolder> seen_loader(
      file_path,
      *load_seen_containers);
    seen_loader.load();

    SeenUserInfoHolder result;
    auto container = load_seen_containers->get(result, key3);
    EXPECT_TRUE(container);
    EXPECT_EQ(result, seen_holder3);
    EXPECT_TRUE(container->get(result, key3));
    EXPECT_EQ(result, seen_holder3);
  }
}

class PortionsTest : public testing::Test
{
protected:
  struct BoundData
  {
    StringDefHashAdapter prefix_key;
    ExternalIdHashAdapter suffix_key;
    BoundUserInfoHolder holder;
  };

  struct SeenData
  {
    HashHashAdapter key;
    SeenUserInfoHolder holder;
  };

protected:
  void SetUp() override
  {
    remove_directory(directory + "/" + seen_name);
    remove_directory(directory + "/" + bound_name);
    remove_directory(rocksdb_path);

    logger = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::EMERGENCY));

    portions = std::make_shared<Portions>(
      logger.in(),
      portions_number,
      directory,
      seen_name,
      bound_name,
      rocksdb_path,
      rocksdb_number_threads,
      rocksdb_compaction_style,
      rocksdb_block_сache_size_mb,
      rocksdb_ttl,
      actual_fetch_size,
      max_fetch_size,
      [] (const std::size_t) {return true;},
      true);
  }

  std::string directory = "/tmp";
  std::string seen_name = "seen";
  std::string bound_name = "bound";
  const std::string rocksdb_path = "/tmp/rocksdb_test";
  const std::size_t portions_number = 17;
  const std::size_t rocksdb_number_threads = 5;
  const rocksdb::CompactionStyle rocksdb_compaction_style =
    rocksdb::CompactionStyle::kCompactionStyleLevel;
  const std::uint32_t rocksdb_block_сache_size_mb = 100;
  const std::uint32_t rocksdb_ttl = 100;
  const std::size_t actual_fetch_size = 2;
  const std::size_t max_fetch_size = 2;

  Logging::Logger_var logger;
  PortionsPtr portions;
};

TEST_F(PortionsTest, Load1)
{
  std::list<BoundData> bound_datas;
  std::list<SeenData> seen_datas;
  for (std::size_t i = 1; i <= 10000; ++i)
  {
    {
      std::string prefix_key = std::to_string(i);
      std::string suffix_key = std::to_string(i * 10);
      std::string full_key = prefix_key + "/" + suffix_key;

      BoundUserInfoHolder bound_holder;
      bound_holder.user_id = Generics::Uuid::create_random_based();
      bound_holder.flags = 'y';
      bound_holder.bad_event_count = i;
      bound_holder.last_bad_event_day = 2 * i;

      const auto full_hash = portions->hash(full_key);
      const auto prefix_hash = portions->hash(prefix_key);
      const auto suffix_hash = portions->hash(suffix_key);

      auto portion = portions->portion(full_hash);

      BoundData bound_data;
      bound_data.holder = bound_holder;
      bound_data.prefix_key = StringDefHashAdapter(prefix_key, prefix_hash);
      bound_data.suffix_key = ExternalIdHashAdapter(suffix_key, suffix_hash);
      bound_datas.emplace_back(bound_data);

      portion->set_bound(bound_data.prefix_key, bound_data.suffix_key, bound_data.holder);

      SeenUserInfoHolder seen_holder;
      seen_holder.first_seen_time_offset_ = 2 * i;

      SeenData seen_data;
      seen_data.holder = seen_holder;
      seen_data.key = HashHashAdapter(full_hash);
      seen_datas.emplace_back(seen_data);

      portion->set_seen(seen_data.key, seen_data.holder);
    }
  }

  EXPECT_NO_THROW(portions->save(FilterDate{10}));
  portions.reset();

  {
    auto new_portions = std::make_shared<Portions>(
      logger.in(),
      portions_number,
      directory,
      seen_name,
      bound_name,
      rocksdb_path,
      rocksdb_number_threads,
      rocksdb_compaction_style,
      rocksdb_block_сache_size_mb,
      rocksdb_ttl,
      actual_fetch_size,
      max_fetch_size,
      [] (const std::size_t) {return true;},
      true);

    for (const auto& data : bound_datas)
    {
      std::string full_key = std::string(data.prefix_key.str()) + "/" + data.suffix_key.str();
      const auto full_hash = new_portions->hash(full_key);
      auto portion = new_portions->portion(full_hash);

      BoundUserInfoHolder result;
      auto container = portion->get_bound(result, data.prefix_key, data.suffix_key);
      EXPECT_TRUE(container);
      EXPECT_EQ(result, data.holder);
      EXPECT_TRUE(container->get(result,data.suffix_key));
      EXPECT_EQ(result, data.holder);
    }

    for (const auto& data : seen_datas)
    {
      auto portion = new_portions->portion(data.key.hash());

      SeenUserInfoHolder result;
      auto container = portion->get_seen(result, data.key);
      EXPECT_TRUE(container);
      EXPECT_EQ(result, data.holder);
      EXPECT_TRUE(container->get(result,data.key));
      EXPECT_EQ(result, data.holder);
    }
  }
}

TEST_F(PortionsTest, Load2)
{
  std::list<BoundData> bound_datas;
  std::list<SeenData> seen_datas;
  for (std::size_t i = 1; i <= 10; ++i)
  {
    {
      std::string prefix_key = std::to_string(i);
      std::string suffix_key = std::to_string(i * 10);
      std::string full_key = prefix_key + "/" + suffix_key;

      BoundUserInfoHolder bound_holder;
      bound_holder.user_id = Generics::Uuid::create_random_based();
      bound_holder.flags = 'y';
      bound_holder.bad_event_count = i;
      bound_holder.last_bad_event_day = 2 * i;

      const auto full_hash = portions->hash(full_key);
      const auto prefix_hash = portions->hash(prefix_key);
      const auto suffix_hash = portions->hash(suffix_key);

      auto portion = portions->portion(full_hash);

      BoundData bound_data;
      bound_data.holder = bound_holder;
      bound_data.prefix_key = StringDefHashAdapter(prefix_key, prefix_hash);
      bound_data.suffix_key = ExternalIdHashAdapter(suffix_key, suffix_hash);
      bound_datas.emplace_back(bound_data);

      portion->set_bound(bound_data.prefix_key, bound_data.suffix_key, bound_data.holder);
      portion->erase_bound(bound_data.prefix_key, bound_data.suffix_key);

      SeenUserInfoHolder seen_holder;
      seen_holder.first_seen_time_offset_ = i;

      SeenData seen_data;
      seen_data.holder = seen_holder;
      seen_data.key = HashHashAdapter(full_hash);
      seen_datas.emplace_back(seen_data);

      portion->set_seen(seen_data.key, seen_data.holder);
      portion->erase_seen(seen_data.key);
    }
  }

  EXPECT_NO_THROW(portions->save(FilterDate{10}));
  portions.reset();
  remove_directory(directory + "/" + seen_name);
  remove_directory(directory + "/" + bound_name);

  {
    auto new_portions = std::make_shared<Portions>(
      logger.in(),
      portions_number,
      directory,
      seen_name,
      bound_name,
      rocksdb_path,
      rocksdb_number_threads,
      rocksdb_compaction_style,
      rocksdb_block_сache_size_mb,
      rocksdb_ttl,
      actual_fetch_size,
      max_fetch_size,
      [] (const std::size_t) {return true;},
      true);

    for (const auto& data : bound_datas)
    {
      std::string full_key = std::string(data.prefix_key.str()) + "/" + data.suffix_key.str();
      const auto full_hash = new_portions->hash(full_key);
      auto portion = new_portions->portion(full_hash);

      BoundUserInfoHolder result;
      auto container = portion->get_bound(result, data.prefix_key, data.suffix_key);
      EXPECT_FALSE(container);
    }

    for (const auto& data : seen_datas)
    {
      auto portion = new_portions->portion(data.key.hash());

      SeenUserInfoHolder result;
      auto container = portion->get_seen(result, data.key);
      EXPECT_FALSE(container);
    }
  }
}

TEST_F(PortionsTest, Rocksdb)
{
  portions.reset();
  portions = std::make_shared<Portions>(
    logger.in(),
    portions_number,
    directory,
    seen_name,
    bound_name,
    rocksdb_path,
    rocksdb_number_threads,
    rocksdb_compaction_style,
    rocksdb_block_сache_size_mb,
    rocksdb_ttl,
    actual_fetch_size,
    max_fetch_size,
    [] (const std::size_t) {return true;},
    true);

  std::list<BoundData> bound_datas;
  std::list<SeenData> seen_datas;
  for (std::size_t i = 1; i <= 10; ++i)
  {
    {
      std::string prefix_key = std::to_string(i);
      std::string suffix_key = std::to_string(i * 10);
      std::string full_key = prefix_key + "/" + suffix_key;

      BoundUserInfoHolder bound_holder;
      bound_holder.user_id = Generics::Uuid::create_random_based();
      bound_holder.flags = 'y';
      bound_holder.bad_event_count = i;
      bound_holder.last_bad_event_day = 2 * i;

      const auto full_hash = portions->hash(full_key);
      const auto prefix_hash = portions->hash(prefix_key);
      const auto suffix_hash = portions->hash(suffix_key);

      auto portion = portions->portion(full_hash);

      BoundData bound_data;
      bound_data.holder = bound_holder;
      bound_data.prefix_key = StringDefHashAdapter(prefix_key, prefix_hash);
      bound_data.suffix_key = ExternalIdHashAdapter(suffix_key, suffix_hash);
      bound_datas.emplace_back(bound_data);

      portion->set_bound(bound_data.prefix_key, bound_data.suffix_key, bound_data.holder);

      SeenUserInfoHolder seen_holder;
      seen_holder.first_seen_time_offset_ = i;

      SeenData seen_data;
      seen_data.holder = seen_holder;
      seen_data.key = HashHashAdapter(full_hash);
      seen_datas.emplace_back(seen_data);

      portion->set_seen(seen_data.key, seen_data.holder);
    }
  }

  EXPECT_NO_THROW(portions->save(FilterDate{0}));
  portions.reset();

  {
    auto portions = std::make_shared<Portions>(
      logger.in(),
      portions_number,
      directory,
      seen_name,
      bound_name,
      rocksdb_path,
      rocksdb_number_threads,
      rocksdb_compaction_style,
      rocksdb_block_сache_size_mb,
      rocksdb_ttl,
      actual_fetch_size,
      max_fetch_size,
      [] (const std::size_t) {return true;},
      true);

    for (const auto& data : bound_datas)
    {
      std::string full_key = std::string(data.prefix_key.str()) + "/" + data.suffix_key.str();
      const auto full_hash = portions->hash(full_key);
      auto portion = portions->portion(full_hash);

      BoundUserInfoHolder result;
      auto container = portion->get_bound(result, data.prefix_key, data.suffix_key);
      EXPECT_TRUE(container);
      EXPECT_EQ(result, data.holder);
      EXPECT_TRUE(container->get(result,data.suffix_key));
      EXPECT_EQ(result, data.holder);
    }

    for (const auto& data : seen_datas)
    {
      auto portion = portions->portion(data.key.hash());

      SeenUserInfoHolder result;
      auto container = portion->get_seen(result, data.key);
      EXPECT_TRUE(container);
      EXPECT_EQ(result, data.holder);
      EXPECT_TRUE(container->get(result,data.key));
      EXPECT_EQ(result, data.holder);
    }
  }
}

TEST(Comparison, UserBindChunk)
{
  Logging::Logger_var logger =
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::EMERGENCY));

  const std::uint16_t seen_default_expire_time = 10;
  const std::uint16_t bound_default_expire_time = 10;
  const UserBindTwoLayersChunk::ListSourceExpireTime bound_list_source_expire_time;
  const std::string directory = "/tmp/test_chunk_two_layers";
  const std::string seen_name = "seen";
  const std::string bound_name = "bound";
  const Generics::Time min_age(1);
  const bool bind_at_min_age = true;
  const std::size_t max_bad_event = 100;
  const bool load_slave = true;
  const std::size_t portions_number = 10;
  const std::size_t partition_index = 0;
  const std::size_t partitions_number = 1;
  const std::size_t rocksdb_number_threads = 5;
  const rocksdb::CompactionStyle rocksdb_compaction_style = rocksdb::CompactionStyle::kCompactionStyleLevel;
  const std::uint32_t rocksdb_block_сache_size_mb = 10;
  const std::uint32_t rocksdb_ttl = 8640000;

  std::filesystem::remove_all(directory);

  UserBindTwoLayersChunk_var user_bind_chunk_two_layers =
    new UserBindTwoLayersChunk(
      logger,
      seen_default_expire_time,
      bound_default_expire_time,
      bound_list_source_expire_time,
      directory,
      seen_name,
      bound_name,
      min_age,
      bind_at_min_age,
      max_bad_event,
      portions_number,
      load_slave,
      partition_index,
      partitions_number,
      rocksdb_number_threads,
      rocksdb_compaction_style,
      rocksdb_block_сache_size_mb,
      rocksdb_ttl);

  const std::string file_root = "/tmp/test_chunk";
  const std::string file_prefix = "seen";
  const std::string bound_file_prefix = "bound";
  const Generics::Time extend_time_period(604800);
  const Generics::Time bound_extend_time_period(7257600);

  std::filesystem::remove_all(file_root);
  std::filesystem::create_directories(file_root);

  UserBindChunk_var user_bind_chunk =
    new UserBindChunk(
      logger,
      file_root.c_str(),
      file_prefix.c_str(),
      bound_file_prefix.c_str(),
      extend_time_period,
      bound_extend_time_period,
      min_age,
      bind_at_min_age,
      max_bad_event,
      portions_number,
      load_slave,
      partition_index,
      partitions_number,
      1);

  const std::size_t total_number = 3;
  const std::size_t total_user = 10;
  const std::size_t total_id_prefix = 5;
  const std::size_t total_id_suffix = 5;
  const std::size_t total_day = 10;

  std::list<Generics::Uuid> user_ids;
  for (std::size_t number = 1; number <= total_number; number += 1)
  {
    for (std::size_t user = 1; user <= total_user; user += 1)
    {
      const Generics::Uuid user_id = Generics::Uuid::create_random_based();
      user_ids.emplace_back(user_id);
      for (std::size_t id_prefix = 1; id_prefix <= total_id_prefix; id_prefix += 1)
      {
        for (std::size_t id_suffix = 1; id_suffix <= total_id_suffix; id_suffix += 1)
        {
          const std::string external_id = "prefix" + std::to_string(id_prefix) +
                                          "/" + "suffix" + std::to_string(id_suffix);
          for (int resave_if_exists = 0; resave_if_exists <= 1; resave_if_exists += 1)
          {
            for (int ignore_bad_event = 0; ignore_bad_event <= 1; ignore_bad_event += 1)
            {
              Generics::Time now(
                String::SubString("2010-01-01"),
                "%Y-%m-%d");
              for (std::size_t day = 1; day <= total_day; day += 1)
              {
                now += Generics::Time::ONE_DAY * 10;

                auto add_user_info = user_bind_chunk->add_user_id(
                  external_id,
                  user_id,
                  now,
                  resave_if_exists,
                  ignore_bad_event);

                auto add_user_info_two_layers = user_bind_chunk_two_layers->add_user_id(
                  external_id,
                  user_id,
                  now,
                  resave_if_exists,
                  ignore_bad_event);

                EXPECT_EQ(add_user_info.user_id, add_user_info_two_layers.user_id);
                EXPECT_EQ(add_user_info.min_age_reached, add_user_info_two_layers.min_age_reached);
                EXPECT_EQ(add_user_info.user_id_generated, add_user_info_two_layers.user_id_generated);
                EXPECT_EQ(add_user_info.created, add_user_info_two_layers.created);
                EXPECT_EQ(add_user_info.invalid_operation, add_user_info_two_layers.invalid_operation);
                EXPECT_EQ(add_user_info.user_found, add_user_info_two_layers.user_found);

                for (int silent = 0; silent <= 1; silent += 1)
                {
                  for (int for_set_cookie = 0; for_set_cookie <= 1; for_set_cookie += 1)
                  {
                    auto get_user_info = user_bind_chunk->get_user_id(
                      external_id,
                      user_id,
                      now,
                      silent,
                      now,
                      for_set_cookie);

                    auto get_user_info_two_layers = user_bind_chunk_two_layers->get_user_id(
                      external_id,
                      user_id,
                      now,
                      silent,
                      now,
                      for_set_cookie);

                    EXPECT_EQ(get_user_info.user_id, get_user_info_two_layers.user_id);
                    EXPECT_EQ(get_user_info.min_age_reached, get_user_info_two_layers.min_age_reached);
                    EXPECT_EQ(get_user_info.user_id_generated, get_user_info_two_layers.user_id_generated);
                    EXPECT_EQ(get_user_info.created, get_user_info_two_layers.created);
                    EXPECT_EQ(get_user_info.invalid_operation, get_user_info_two_layers.invalid_operation);
                    EXPECT_EQ(get_user_info.user_found, get_user_info_two_layers.user_found);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  for (std::size_t i = 1; i <= 10; i += 1)
  {
    user_ids.emplace_back(Generics::Uuid::create_random_based());
  }

  std::unordered_map<std::string, Generics::Uuid> generated_uuid;
  std::unordered_map<std::string, Generics::Uuid> generated_uuid_two_layers;

  for (const auto& user_id : user_ids)
  {
    for (std::size_t id_prefix = 1; id_prefix <= total_id_prefix + 3; id_prefix += 1)
    {
      for (std::size_t id_suffix = 1; id_suffix <= total_id_suffix + 3; id_suffix += 1)
      {
        const std::string external_id = "prefix" + std::to_string(id_prefix) +
          "/" + "suffix" + std::to_string(id_suffix);
        Generics::Time now(
          String::SubString("2010-01-01"),
          "%Y-%m-%d");
        for (std::size_t day = 1; day <= total_day + 20; day += 1)
        {
          now += Generics::Time::ONE_DAY * 10;

          for (int silent = 0; silent <= 1; silent += 1)
          {
            for (int for_set_cookie = 0; for_set_cookie <= 1; for_set_cookie += 1)
            {
              auto get_user_info = user_bind_chunk->get_user_id(
                external_id,
                user_id,
                now,
                silent,
                now,
                for_set_cookie);

              auto get_user_info_two_layers = user_bind_chunk_two_layers->get_user_id(
                external_id,
                user_id,
                now,
                silent,
                now,
                for_set_cookie);

              if (get_user_info.user_id_generated)
              {
                generated_uuid[external_id] = get_user_info.user_id;
              }

              if (get_user_info_two_layers.user_id_generated)
              {
                generated_uuid_two_layers[external_id] = get_user_info_two_layers.user_id;
              }

              if (!get_user_info.user_id_generated && !get_user_info_two_layers.user_id_generated)
              {
                auto it = generated_uuid.find(external_id);
                if (it != std::end(generated_uuid))
                {
                  EXPECT_EQ(get_user_info.user_id, it->second);
                }

                auto it_two_layers = generated_uuid_two_layers.find(external_id);
                if (it_two_layers !=std::end(generated_uuid_two_layers))
                {
                  EXPECT_EQ(get_user_info_two_layers.user_id, it_two_layers->second);
                }

                if (it == std::end(generated_uuid) && it_two_layers ==std::end(generated_uuid_two_layers))
                {
                  EXPECT_EQ(get_user_info.user_id, get_user_info_two_layers.user_id);
                }
              }

              EXPECT_EQ(get_user_info.min_age_reached, get_user_info_two_layers.min_age_reached);
              EXPECT_EQ(get_user_info.user_id_generated, get_user_info_two_layers.user_id_generated);
              EXPECT_EQ(get_user_info.created, get_user_info_two_layers.created);
              EXPECT_EQ(get_user_info.invalid_operation, get_user_info_two_layers.invalid_operation);
              EXPECT_EQ(get_user_info.user_found, get_user_info_two_layers.user_found);
            }
          }
        }
      }
    }
  }
}

TEST(Filter, Source)
{
  std::uint16_t seen_default_expire_time = 5;
  std::uint16_t bound_default_expire_time = 10;

  SourcesExpireTime::ListSourceExpireTime list_source_expire_time;
  const std::string source = "Y";
  const std::uint16_t expire_time = 100;
  list_source_expire_time.emplace_back(source, expire_time);

  auto sources_expire_time = std::make_shared<SourcesExpireTime>(
    seen_default_expire_time,
    bound_default_expire_time,
    list_source_expire_time);

  FilterSourceDate filter(sources_expire_time);

  SeenUserInfoHolder seen_holder;
  EXPECT_TRUE(filter(seen_holder));
  seen_holder.initial_day_ -= seen_default_expire_time + 1;
  EXPECT_FALSE(filter(seen_holder));

  BoundUserInfoHolder bound_holder;
  EXPECT_TRUE(filter(bound_holder, ""));
  bound_holder.initial_day -= bound_default_expire_time + 1;
  EXPECT_FALSE(filter(bound_holder, ""));
  EXPECT_TRUE(filter(bound_holder, source));

  bound_holder.initial_day -= expire_time + 1;
  EXPECT_FALSE(filter(bound_holder, source));
}