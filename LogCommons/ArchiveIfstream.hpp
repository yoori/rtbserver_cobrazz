#ifndef AD_SERVER_LOG_PROCESSING_ARCHIVE_IFSTREAM_HPP
#define AD_SERVER_LOG_PROCESSING_ARCHIVE_IFSTREAM_HPP

// BOOST
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>

// STD
#include <fstream>

// UNIXCOMMONS
#include <eh/Exception.hpp>

namespace AdServer::LogProcessing
{

struct GzipDecompressorParams final
{
  std::streamsize buffer_size = boost::iostreams::default_device_buffer_size;
  int gzip_window_bits = boost::iostreams::gzip::default_window_bits;
};

struct Bz2DecompressorParams final
{
  std::streamsize buffer_size = boost::iostreams::default_device_buffer_size;
  bool bz2_small = boost::iostreams::bzip2::default_small;
};

struct DecompressorParams final
{
  GzipDecompressorParams gzip_decompressor_params;
  Bz2DecompressorParams bz2_decompressor_params;
};

class ArchiveIfstream final : public boost::iostreams::filtering_istream
{
private:
  using FileDescriptorSource = boost::iostreams::file_descriptor_source;

public:
  enum class ReaderType
  {
    Ifstream,
    FileDescriptorSource
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  ArchiveIfstream(
    const std::string& file_path,
    const std::ios_base::openmode mode = std::ios::in,
    const DecompressorParams& params = DecompressorParams{},
    const ReaderType reader_type = ReaderType::FileDescriptorSource) noexcept;

  ArchiveIfstream(const ArchiveIfstream&) = delete;
  ArchiveIfstream(ArchiveIfstream&&) = delete;
  ArchiveIfstream& operator=(const ArchiveIfstream&) = delete;
  ArchiveIfstream& operator=(ArchiveIfstream&&) = delete;

  ~ArchiveIfstream() override;

  static bool is_archive(const std::string& file_path);

private:
  static constexpr std::string_view GZIP_EXTENSION = ".gz";
  static constexpr std::string_view BZ2_EXTENSION = ".bz2";

  std::unique_ptr<std::ifstream> ifstream_;

  std::unique_ptr<FileDescriptorSource> file_descriptor_source_;
};

} // namespace AdServer::LogProcessing

#endif //AD_SERVER_LOG_PROCESSING_ARCHIVE_IFSTREAM_HPP