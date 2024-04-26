// STD
#include <cmath>
#include <filesystem>
#include <iostream>
#include <unordered_map>

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Utils.hpp>
#include <UServerUtils/Grpc/RocksDB/DataBase.hpp>

enum class TestType
{
  Hash,
  Rocksdb
};

class MemoryTest final
{
private:
  using Key = std::string;
  using Value = std::string;
  using Hash = std::unordered_map<Key, Value>;
  using HashPtr = std::unique_ptr<Hash>;
  using DataBase = UServerUtils::Grpc::RocksDB::DataBase;
  using DataBasePtr = std::unique_ptr<DataBase>;

public:
  explicit MemoryTest(
    const TestType test_type,
    const std::uint64_t elements_number,
    const std::size_t key_size,
    const std::size_t value_size,
    const std::size_t block_сache_size_mb)
    : test_type_(test_type),
      elements_number_(elements_number),
      value_(value_size, '7'),
      key_(key_size, '0')
  {
    if (elements_number > std::pow(10, key_size))
    {
      throw std::runtime_error("Elements number is to large");
    }

    if (test_type == TestType::Hash)
    {
      hash_ = std::make_unique<Hash>();
      hash_->reserve(elements_number_ * 5);
    }
    else
    {
      std::size_t number_threads = std::thread::hardware_concurrency();
      if (number_threads == 0)
      {
        number_threads = 16;
      }

      const std::size_t ttl = 1000;
      const std::string db_path = "/tmp/rocksdb_memory_test";
      remove_directory(db_path);

      Logging::Logger_var logger = new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::EMERGENCY));

      rocksdb::DBOptions db_options;
      db_options.IncreaseParallelism(number_threads);
      db_options.create_if_missing = true;

      rocksdb::ColumnFamilyOptions column_family_options;
      column_family_options.OptimizeForPointLookup(block_сache_size_mb);
      column_family_options.compaction_style = rocksdb::kCompactionStyleLevel;
      column_family_options.ttl = ttl;

      std::optional<std::vector<int>> ttls{{static_cast<int>(ttl)}};

      std::vector <rocksdb::ColumnFamilyDescriptor> descriptors{
        {rocksdb::kDefaultColumnFamilyName, column_family_options}
      };

      data_base_ = std::make_unique<DataBase>(
        logger,
        db_path,
        db_options,
        descriptors,
        true,
        ttls);
    }
  }

  void run()
  {
    switch (test_type_)
    {
    case TestType::Hash:
      do_hash_test();
      break;
    case TestType::Rocksdb:
      do_rocksdb_test();
      break;
    }
  }

private:
  void do_hash_test()
  {
    for (std::size_t i = 0; i < elements_number_; ++i)
    {
      hash_->try_emplace(create_key(), value_);
    }
  }

  void do_rocksdb_test()
  {
    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;
    write_options.sync = false;

    for (std::size_t i = 0; i < elements_number_; ++i)
    {
      const auto key = create_key();
      data_base_->get().Put(
        write_options,
        rocksdb::Slice(key.data(), key.size()),
        rocksdb::Slice(value_.data(), value_.size()));
    }
  }

  std::string create_key()
  {
    const std::string result = key_;
    const std::size_t size = key_.size();
    for (int i = size - 1; i >= 0; i -= 1)
    {
      uint8_t digit = key_[i] - '0';
      digit = (digit + 1) % 10;
      key_[i] = '0' + digit;
      if (digit != 0)
        break;
    }

    return result;
  }

  void remove_directory(const std::string& str_path)
  {
    std::filesystem::path path(str_path);
    std::filesystem::remove_all(path);
  }

private:
  const TestType test_type_;

  const std::uint64_t elements_number_;

  const std::string value_;

  std::string key_;

  HashPtr hash_;

  DataBasePtr data_base_;
};

int main(int /*argc*/, char** /*argv*/)
{
  using SignalCatcher = UServerUtils::Grpc::Utils::SignalCatcher;

  try
  {
    const TestType test_type = TestType::Rocksdb;
    const std::uint64_t elements_number = 10000000;
    const std::size_t key_size = 24;
    const std::size_t value_size = 20;

    const std::size_t block_сache_size_mb = 1000;

    MemoryTest test(
      test_type,
      elements_number,
      key_size,
      value_size,
      block_сache_size_mb);
    test.run();

    SignalCatcher signal_catcher{SIGINT, SIGTERM, SIGQUIT};
    std::cout << "\nPress Ctrl+C to finish..." << std::endl;
    signal_catcher.catch_signal();

    return EXIT_SUCCESS;
  }
  catch (const std::exception& exc)
  {
    std::cerr << "Test is failed. Reason="
              << exc.what();
  }
  catch (...)
  {
    std::cerr << "Test is failed. Unknown error";
  }

  return EXIT_FAILURE;
}