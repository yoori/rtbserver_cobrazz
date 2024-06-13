// GTEST
#include <gtest/gtest.h>

//THIS
#include <UserInfoSvcs/UserBindServer/FetchableHashTable.hpp>

class StringHashAdapter final
{
public:
  StringHashAdapter() noexcept
  {
  }

  StringHashAdapter(const char* data) noexcept
    : data_(data),
      hash_(std::hash<std::string_view>{}(data_))
  {
  }

  StringHashAdapter(std::string_view data) noexcept
    : data_(data),
      hash_(std::hash<std::string_view>{}(data))
  {
  }

  StringHashAdapter(const std::string& data) noexcept
    : data_(data),
      hash_(std::hash<std::string_view>{}(data_))
  {
  }

  StringHashAdapter(const StringHashAdapter& init) = default;
  StringHashAdapter(StringHashAdapter&& init) = default;
  StringHashAdapter& operator=(const StringHashAdapter&) = default;
  StringHashAdapter& operator=(StringHashAdapter&&) = default;

  std::string_view data() const noexcept
  {
    return data_;
  }

  std::size_t hash() const noexcept
  {
    return hash_;
  }

  bool operator==(const StringHashAdapter& other) const noexcept
  {
    return data_ == other.data_;
  }

protected:
  std::string_view data_;

  std::size_t hash_ = 0;
};

TEST(FetchableHashTable, Test1)
{
  AdServer::UserInfoSvcs::USFetchableHashTable<StringHashAdapter, int> hash_table;

  const std::string key1 = "1";
  const int value1 = 1;
  const std::string key2 = "2";
  const int value2 = 2;
  const std::string key3 = "3";
  const int value3 = 3;
  const std::string key4 = "4";
  const int value4 = 4;

  int result = 0;

  for (int i = 1; i <= 100; ++i)
  {
    hash_table.set(key1, value1);
    EXPECT_TRUE(hash_table.get(result, key1));
    EXPECT_EQ(result, value1);
    hash_table.set(key2, value2);
    EXPECT_TRUE(hash_table.get(result, key2));
    EXPECT_EQ(result, value2);
    hash_table.set(key3, value3);
    EXPECT_TRUE(hash_table.get(result, key3));
    EXPECT_EQ(result, value3);

    EXPECT_TRUE(hash_table.erase(key1));
    EXPECT_TRUE(hash_table.erase(key2));
    EXPECT_TRUE(hash_table.erase(key3));

    EXPECT_FALSE(hash_table.erase(key1));
    EXPECT_FALSE(hash_table.erase(key2));
    EXPECT_FALSE(hash_table.erase(key3));

    hash_table.set(key1, value1);
    hash_table.set(key2, value2);
    hash_table.set(key3, value3);
    hash_table.set(key4, value4);

    EXPECT_TRUE(hash_table.get(result, key1));
    EXPECT_EQ(result, value1);
    EXPECT_TRUE(hash_table.get(result, key2));
    EXPECT_EQ(result, value2);
    EXPECT_TRUE(hash_table.get(result, key3));
    EXPECT_EQ(result, value3);
    EXPECT_TRUE(hash_table.get(result, key4));
    EXPECT_EQ(result, value4);

    hash_table.set(key1, value1 * 2);
    hash_table.set(key2, value2 * 2);
    hash_table.set(key3, value3 * 2);
    hash_table.set(key4, value4 * 2);

    EXPECT_TRUE(hash_table.get(result, key1));
    EXPECT_EQ(result, value1 * 2);
    EXPECT_TRUE(hash_table.get(result, key2));
    EXPECT_EQ(result, value2 * 2);
    EXPECT_TRUE(hash_table.get(result, key3));
    EXPECT_EQ(result, value3 * 2);
    EXPECT_TRUE(hash_table.get(result, key4));
    EXPECT_EQ(result, value4 * 2);

    EXPECT_TRUE(hash_table.erase(key1));
    EXPECT_TRUE(hash_table.erase(key2));
    EXPECT_TRUE(hash_table.erase(key3));
    EXPECT_TRUE(hash_table.erase(key4));
  }
}

TEST(FetchableHashTable, Test2)
{
  AdServer::UserInfoSvcs::USFetchableHashTable<StringHashAdapter, int> hash_table;

  std::size_t max_count = 100;
  std::vector<std::string> keys;
  const std::size_t max_elements = 3 * (1 + max_count) * max_count / 2;
  keys.reserve(max_elements);
  std::vector<int> values;
  values.reserve(max_elements);

  for (std::size_t count = 1; count <= max_count; ++count)
  {
    for (std::size_t i = 0; i < count; ++i)
    {
      keys.emplace_back(std::to_string(i));
      values.emplace_back(i * 1000 + 7);
      hash_table.set(keys.back(), values.back());
    }

    std::size_t size = keys.size();
    for (std::size_t i = 0; i < count; ++i)
    {
      int result = 0;
      EXPECT_TRUE(hash_table.get(result, keys[size - i - 1]));
      EXPECT_EQ(result, values[size - i - 1]);
    }

    for (std::size_t i = 0; i < count; ++i)
    {
      EXPECT_TRUE(hash_table.erase(keys[size - i - 1]));
    }

    for (std::size_t i = 0; i < count; ++i)
    {
      EXPECT_FALSE(hash_table.erase(keys[size - i - 1]));
    }

    const std::size_t count2 = count * 2;
    for (std::size_t i = 0; i < count2; ++i)
    {
      keys.emplace_back(std::to_string(i));
      values.emplace_back(i * 1000 + 7);
      hash_table.set(keys.back(), values.back());
    }

    size = keys.size();
    for (std::size_t i = 0; i < count2; ++i)
    {
      int result = 0;
      EXPECT_TRUE(hash_table.get(result, keys[size - i - 1]));
      EXPECT_EQ(result, values[size - i - 1]);
    }
  }
}