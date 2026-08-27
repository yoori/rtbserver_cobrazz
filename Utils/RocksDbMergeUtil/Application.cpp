#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>
#include <rocksdb/write_batch.h>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
  constexpr std::uint64_t WRITE_BATCH_RECORDS_LIMIT = 10000;
  constexpr std::uint64_t WRITE_BATCH_BYTES_LIMIT = 64ULL * 1024ULL * 1024ULL;

  int
  usage()
  {
    std::cerr
      << "usage: RocksDbMergeUtil --output <output-db-dir> "
         "<input-db-dir> [<input-db-dir>...]\n"
      << "\n"
      << "Output directory must already exist and be empty.\n"
      << "If an input key is present in more than one DB, any value may be kept.\n";
    return 2;
  }

  rocksdb::Options
  make_options()
  {
    rocksdb::Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.compression = rocksdb::kNoCompression;
    options.target_file_size_multiplier = 2;

    rocksdb::BlockBasedTableOptions table_options;
    table_options.checksum = rocksdb::kNoChecksum;
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

    return options;
  }

  void
  check_input_path(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("input path doesn't exist: " + path.string());
    }

    if (!std::filesystem::is_directory(path))
    {
      throw std::runtime_error("input path isn't a directory: " + path.string());
    }
  }

  void
  check_empty_output_path(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("output path doesn't exist: " + path.string());
    }

    if (!std::filesystem::is_directory(path))
    {
      throw std::runtime_error("output path isn't a directory: " + path.string());
    }

    if (!std::filesystem::is_empty(path))
    {
      throw std::runtime_error("output path isn't empty: " + path.string());
    }
  }

  std::unique_ptr<rocksdb::DB>
  open_input_db(const std::filesystem::path& path)
  {
    rocksdb::Options options = make_options();
    options.create_if_missing = false;

    std::unique_ptr<rocksdb::DB> db;
    const auto status = rocksdb::DB::OpenForReadOnly(options, path.string(), &db);
    if (!status.ok())
    {
      throw std::runtime_error("can't open input DB " + path.string() + ": " + status.ToString());
    }

    return db;
  }

  std::unique_ptr<rocksdb::DB>
  open_output_db(const std::filesystem::path& path)
  {
    rocksdb::Options options = make_options();
    options.create_if_missing = true;

    std::unique_ptr<rocksdb::DB> db;
    const auto status = rocksdb::DB::Open(options, path.string(), &db);
    if (!status.ok())
    {
      throw std::runtime_error("can't open output DB " + path.string() + ": " + status.ToString());
    }

    return db;
  }

  struct MergeStats
  {
    std::uint64_t inputs = 0;
    std::uint64_t records = 0;
    std::uint64_t bytes = 0;
  };

  void
  write_batch(
    rocksdb::DB& output_db,
    rocksdb::WriteBatch& batch,
    std::uint64_t& batch_records,
    std::uint64_t& batch_bytes)
  {
    if (batch_records == 0)
    {
      return;
    }

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;

    const auto status = output_db.Write(write_options, &batch);
    if (!status.ok())
    {
      throw std::runtime_error("can't write output DB: " + status.ToString());
    }

    batch.Clear();
    batch_records = 0;
    batch_bytes = 0;
  }

  void
  merge_input_db(rocksdb::DB& output_db, const std::filesystem::path& input_path, MergeStats& stats)
  {
    auto input_db = open_input_db(input_path);

    rocksdb::ReadOptions read_options;
    read_options.verify_checksums = true;
    std::unique_ptr<rocksdb::Iterator> iterator(input_db->NewIterator(read_options));

    rocksdb::WriteBatch batch;
    std::uint64_t batch_records = 0;
    std::uint64_t batch_bytes = 0;

    for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next())
    {
      const auto key = iterator->key();
      const auto value = iterator->value();
      const auto status = batch.Put(key, value);
      if (!status.ok())
      {
        throw std::runtime_error(
          "can't add record from " + input_path.string() +
          " to write batch: " + status.ToString());
      }

      ++stats.records;
      stats.bytes += key.size() + value.size();
      ++batch_records;
      batch_bytes += key.size() + value.size();

      if (batch_records >= WRITE_BATCH_RECORDS_LIMIT || batch_bytes >= WRITE_BATCH_BYTES_LIMIT)
      {
        write_batch(output_db, batch, batch_records, batch_bytes);
      }
    }

    const auto iterator_status = iterator->status();
    if (!iterator_status.ok())
    {
      throw std::runtime_error(
        "can't iterate input DB " + input_path.string() +
        ": " + iterator_status.ToString());
    }

    write_batch(output_db, batch, batch_records, batch_bytes);
    ++stats.inputs;
  }

  void
  flush_output_db(rocksdb::DB& output_db)
  {
    rocksdb::FlushOptions flush_options;
    flush_options.wait = true;
    const auto status = output_db.Flush(flush_options);
    if (!status.ok())
    {
      throw std::runtime_error("can't flush output DB: " + status.ToString());
    }
  }
}

int
main(int argc, char** argv)
{
  try
  {
    if (argc < 4 || std::string(argv[1]) != "--output")
    {
      return usage();
    }

    const std::filesystem::path output_path = argv[2];
    std::vector<std::filesystem::path> input_paths;
    input_paths.reserve(argc - 3);
    for (int i = 3; i < argc; ++i)
    {
      input_paths.emplace_back(argv[i]);
    }

    check_empty_output_path(output_path);
    for (const auto& input_path : input_paths)
    {
      check_input_path(input_path);
    }

    auto output_db = open_output_db(output_path);

    MergeStats stats;
    for (const auto& input_path : input_paths)
    {
      merge_input_db(*output_db, input_path, stats);
    }

    flush_output_db(*output_db);
    const auto close_status = output_db->Close();
    if (!close_status.ok())
    {
      throw std::runtime_error("can't close output DB: " + close_status.ToString());
    }

    std::cout
      << "merged input_dbs=" << stats.inputs
      << " records=" << stats.records
      << " bytes=" << stats.bytes << " output=" << output_path.string() << '\n';

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "RocksDbMergeUtil: " << ex.what() << '\n';
  }
  catch(...)
  {
    std::cerr << "RocksDbMergeUtil: unknown error\n";
  }

  return 1;
}
