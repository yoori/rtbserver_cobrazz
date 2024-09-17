#ifndef AD_SERVER_LOG_PROCESSING_ARCHIVE_OFSTREAM_HPP
#define AD_SERVER_LOG_PROCESSING_ARCHIVE_OFSTREAM_HPP

// BOOST
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

// STD
#include <fstream>
#include <variant>

// UNIXCOMMONS
#include <eh/Exception.hpp>

namespace AdServer::LogProcessing
{

using GzipParams = boost::iostreams::gzip_params;
using Bzip2Params = boost::iostreams::bzip2_params;
using ArchiveParams = std::variant<GzipParams, Bzip2Params>;

namespace Archive
{

extern const GzipParams gzip_default_compression_params;
extern const GzipParams gzip_best_speed_params;
extern const GzipParams gzip_best_compression_params;
extern const Bzip2Params bzip2_default_compression_params;

} // namespace Archive

class ArchiveOfstream final : public boost::iostreams::filtering_ostream
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  ArchiveOfstream(
    const std::string& file_path,
    const GzipParams& params,
    const std::ios_base::openmode mode =
      std::ios_base::out | std::ios_base::app | std::ios_base::binary,
    const std::streamsize buffer_size =
      boost::iostreams::default_device_buffer_size);

  ArchiveOfstream(
    const std::string& file_path,
    const Bzip2Params& params,
    const std::ios_base::openmode mode =
      std::ios_base::out | std::ios_base::app | std::ios_base::binary,
    const std::streamsize buffer_size =
      boost::iostreams::default_device_buffer_size);

  ArchiveOfstream(
    const std::string& file_path,
    const ArchiveParams& params,
    const std::ios_base::openmode mode =
      std::ios_base::out | std::ios_base::app | std::ios_base::binary,
    const std::streamsize buffer_size =
      boost::iostreams::default_device_buffer_size);

  ArchiveOfstream(const ArchiveOfstream&) = delete;
  ArchiveOfstream(ArchiveOfstream&&) = delete;
  ArchiveOfstream& operator=(const ArchiveOfstream&) = delete;
  ArchiveOfstream& operator=(ArchiveOfstream&&) = delete;

  ~ArchiveOfstream() override;

  const std::string& file_path() const noexcept;

  const std::string& file_extension() const noexcept;

private:
  void init(
    const std::string& file_path,
    const GzipParams& params,
    const std::ios_base::openmode mode,
    const std::streamsize buffer_size);

  void init(
    const std::string& file_path,
    const Bzip2Params& params,
    const std::ios_base::openmode mode,
    const std::streamsize buffer_size);

  void open_file(
    const std::string& file_path,
    const std::string_view extension,
    const std::ios_base::openmode mode);

  void check();

private:
  static constexpr std::string_view GZIP_EXTENSION = ".gz";
  static constexpr std::string_view BZ2_EXTENSION = ".bz2";

  std::string file_extension_;

  std::string file_path_;

  std::ofstream ofstream_;
};

} // namespace AdServer::LogProcessing

#endif //AD_SERVER_LOG_PROCESSING_ARCHIVE_OSTREAM_HPP
