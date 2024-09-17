// GTEST
#include <gtest/gtest.h>

// STD
#include <filesystem>
#include <sstream>

// THIS
#include <LogCommons/ArchiveIfstream.hpp>
#include <LogCommons/ArchiveOfstream.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogCommons/LogCommons.hpp>

namespace
{

void clear(const std::string& path)
{
  const std::string path_zlib = path + ".zz";
  std::filesystem::remove(path_zlib);

  const std::string path_gzip = path + ".gz";
  std::filesystem::remove(path_gzip);

  const std::string path_bzip2 = path + ".bz2";
  std::filesystem::remove(path_bzip2);
}

} // namespace

namespace AdServer::LogProcessing
{

using BidTraits = LogDefaultTraits<BidCostStatInnerCollector, false>;
using BidCollector = BidTraits::CollectorType;

template<> const char* BidTraits::B::base_name_ = "BidCostAggStat";
template<> const char* BidTraits::B::signature_ = "BidCostAggStat";;
template<> const char* BidTraits::B::current_version_ = "2.5";

} // namespace AdServer::LogProcessing

TEST(TestGzip, Test)
{
  const std::string file_path = "/tmp/test_file";
  std::string test_data = "Test";
  const std::size_t count = 100;

  clear(file_path);

  std::string test_required;
  for (std::size_t i = 1; i <= count; ++i)
  {
    test_required += test_data;

    boost::iostreams::gzip_params gzip_params;
    if (i == 1)
    {
      gzip_params.level = boost::iostreams::gzip::default_compression;
      gzip_params.strategy = boost::iostreams::gzip::default_strategy;
      gzip_params.calculate_crc = boost::iostreams::gzip::default_crc;
      gzip_params.window_bits = 10;
    }
    else if (i == 2)
    {
      gzip_params.level = boost::iostreams::gzip::best_compression;
      gzip_params.strategy = boost::iostreams::gzip::filtered;
      gzip_params.calculate_crc = !boost::iostreams::gzip::default_crc;
      gzip_params.window_bits = 11;
    }
    else if (i == 3)
    {
      gzip_params.level = boost::iostreams::gzip::best_speed;
      gzip_params.strategy = boost::iostreams::gzip::huffman_only;
      gzip_params.calculate_crc = boost::iostreams::gzip::default_crc;
      gzip_params.window_bits = 12;
    }
    else if (i == 4)
    {
      gzip_params.level = boost::iostreams::gzip::no_compression;
      gzip_params.strategy = boost::iostreams::gzip::huffman_only;
      gzip_params.calculate_crc = boost::iostreams::gzip::default_crc;
      gzip_params.window_bits = 13;
    }
    else
    {
      gzip_params.level = boost::iostreams::gzip::default_compression;
      gzip_params.strategy = boost::iostreams::gzip::default_strategy;
      gzip_params.calculate_crc = boost::iostreams::gzip::default_crc;
      gzip_params.window_bits = 14;
    }

    AdServer::LogProcessing::ArchiveOfstream archive_ofstream(
      file_path,
      gzip_params,
      std::ios_base::out | std::ios_base::app | std::ios_base::binary);

    std::ostream& ostream = archive_ofstream;
    ostream << test_data;

    EXPECT_EQ(archive_ofstream.file_extension(), ".gz");
    EXPECT_EQ(archive_ofstream.file_path(), file_path + ".gz");
    EXPECT_TRUE(ostream);
  }

  {
    AdServer::LogProcessing::ArchiveIfstream archive_ifstream(
      file_path + ".gz");
    EXPECT_TRUE(archive_ifstream);
    std::istream& istream = archive_ifstream;
    std::string data;
    std::string result;
    while (!archive_ifstream.eof())
    {
      data.clear();
      istream >> data;
      result.append(data);
      EXPECT_TRUE(istream);
    }

    EXPECT_EQ(test_required, result);
  }

  {
    AdServer::LogProcessing::ArchiveIfstream archive_ifstream(
      std::string("/tmp/not_existing_file") + ".gz");
    EXPECT_FALSE(archive_ifstream);
  }
}

TEST(Bzip2, Test)
{
  const std::string file_path = "/tmp/test_file";
  std::string test_data = "Test";
  const std::size_t count = 100;

  clear(file_path);

  std::string test_required;
  for (std::size_t i = 1; i <= count; ++i)
  {
    test_required += test_data;

    boost::iostreams::bzip2_params bzip2_params;
    bzip2_params.block_size = i % 9 + 1;
    bzip2_params.work_factor = i % 100 + 1;

    AdServer::LogProcessing::ArchiveOfstream archive_ofstream(
      file_path,
      bzip2_params,
      std::ios_base::out | std::ios_base::app | std::ios_base::binary);
    std::ostream& ostream = archive_ofstream;
    ostream << test_data;

    EXPECT_EQ(archive_ofstream.file_extension(), ".bz2");
    EXPECT_EQ(archive_ofstream.file_path(), file_path + ".bz2");
    EXPECT_TRUE(ostream);
  }

  {
    std::string result;
    AdServer::LogProcessing::ArchiveIfstream archive_ifstream(
      file_path + ".bz2");
    std::istream& istream = archive_ifstream;
    std::string data;
    while (!archive_ifstream.eof())
    {
      data.clear();
      archive_ifstream >> data;
      result.append(data);
      EXPECT_TRUE(istream);
    }

    EXPECT_EQ(test_required, result);
  }
}

TEST(TestGzip, BidCostStat)
{
  using Traits = AdServer::LogProcessing::BidTraits;
  using Collector = AdServer::LogProcessing::BidCollector;
  using Key = Collector::KeyT;
  using Data = Collector::DataT;
  using FixedNum = Key::FixedNum;

  const std::string file_path = "/tmp/test_file";

  clear(file_path);

  Collector collector;
  {
    Key key(1, String::SubString("11"), String::SubString("url1"), FixedNum("0.1"), 111);
    Data data(1111, 11111, 111111);
    collector.add(key, data);
  }
  {
    Key key(2, String::SubString("22"), String::SubString("url2"), FixedNum("0.2"), 222);
    Data data(2222, 22222, 222222);
    collector.add(key, data);
  }
  {
    Key key(3, String::SubString("33"), String::SubString("url3"), FixedNum("0.3"), 333);
    Data data(3333, 33333, 333333);
    collector.add(key, data);
  }

  {
    boost::iostreams::gzip_params gzip_params;
    AdServer::LogProcessing::ArchiveOfstream archive_ofstream(
      file_path,
      gzip_params,
      std::ios_base::out | std::ios_base::app | std::ios_base::binary);

    AdServer::LogProcessing::DefaultSaveStrategy<Traits>().save(archive_ofstream, collector);
    EXPECT_TRUE(archive_ofstream);
  }

  {
    AdServer::LogProcessing::ArchiveIfstream archive_ifstream(
      file_path + ".gz");
    EXPECT_TRUE(archive_ifstream);
    std::istream& istream = archive_ifstream;

    Collector result;
    AdServer::LogProcessing::LogIoProxy<Traits>::load(result, istream);
    EXPECT_EQ(result, collector);
  }
}