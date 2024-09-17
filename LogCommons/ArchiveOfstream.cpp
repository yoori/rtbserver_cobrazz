// STD
#include <filesystem>

// UNIXCOMMONS
#include <Generics/Function.hpp>
#include <Stream/MemoryStream.hpp>

// THIS
#include <LogCommons/ArchiveOfstream.hpp>

namespace AdServer::LogProcessing
{

namespace Archive
{

const GzipParams gzip_default_compression_params = GzipParams{
  boost::iostreams::gzip::default_compression,
  boost::iostreams::gzip::deflated,
  boost::iostreams::gzip::default_window_bits,
  boost::iostreams::gzip::default_mem_level,
  boost::iostreams::gzip::default_strategy,
};

const GzipParams gzip_best_speed_params = GzipParams{
  boost::iostreams::gzip::best_speed,
  boost::iostreams::gzip::deflated,
  boost::iostreams::gzip::default_window_bits,
  boost::iostreams::gzip::default_mem_level,
  boost::iostreams::gzip::default_strategy,
};

const GzipParams gzip_best_compression_params = GzipParams{
  boost::iostreams::gzip::best_compression,
  boost::iostreams::gzip::deflated,
  boost::iostreams::gzip::default_window_bits,
  boost::iostreams::gzip::default_mem_level,
  boost::iostreams::gzip::default_strategy,
};

const Bzip2Params bzip2_default_compression_params = Bzip2Params{
  boost::iostreams::bzip2::default_block_size,
  boost::iostreams::bzip2::default_work_factor
};

} // namespace Archive

ArchiveOfstream::ArchiveOfstream(
  const std::string& file_path,
  const GzipParams& params,
  const std::ios_base::openmode mode,
  const std::streamsize buffer_size)
{
  init(file_path, params, mode, buffer_size);
}

ArchiveOfstream::ArchiveOfstream(
  const std::string& file_path,
  const Bzip2Params& params,
  const std::ios_base::openmode mode,
  const std::streamsize buffer_size)
{
  init(file_path, params, mode, buffer_size);
}

ArchiveOfstream::ArchiveOfstream(
  const std::string& file_path,
  const ArchiveParams& params,
  const std::ios_base::openmode mode,
  const std::streamsize buffer_size)
{
  if (const auto* pval = std::get_if<GzipParams>(&params))
  {
    init(file_path, *pval, mode, buffer_size);
  }
  else if (const auto* pval = std::get_if<Bzip2Params>(&params))
  {
    init(file_path, *pval, mode, buffer_size);
  }
  else
  {
    Stream::Error stream;
    stream << FNS
           << "Params has unknown type";
    throw Exception(stream);
  }
}

ArchiveOfstream::~ArchiveOfstream()
{
  flush();
  reset();
}

const std::string& ArchiveOfstream::file_path() const noexcept
{
  return file_path_;
}

const std::string& ArchiveOfstream::file_extension() const noexcept
{
  return file_extension_;
}

void ArchiveOfstream::init(
  const std::string& file_path,
  const GzipParams& params,
  const std::ios_base::openmode mode,
  const std::streamsize buffer_size)
{
  open_file(file_path, GZIP_EXTENSION, mode);

  GzipParams result_params = params;
  result_params.noheader = false;

  boost::iostreams::gzip_compressor compressor{result_params, buffer_size};
  boost::iostreams::filtering_ostream::push(compressor);
  boost::iostreams::filtering_ostream::push(ofstream_);

  check();
}

void ArchiveOfstream::init(
  const std::string& file_path,
  const Bzip2Params& params,
  const std::ios_base::openmode mode,
  const std::streamsize buffer_size)
{
  open_file(file_path, BZ2_EXTENSION, mode);

  boost::iostreams::bzip2_compressor compressor{params, buffer_size};
  boost::iostreams::filtering_ostream::push(compressor);
  boost::iostreams::filtering_ostream::push(ofstream_);

  check();
}

void ArchiveOfstream::open_file(
  const std::string& file_path,
  const std::string_view extension,
  const std::ios_base::openmode mode)
{
  file_extension_ = extension;
  file_path_ = file_path + file_extension_;
  ofstream_.open(file_path_, mode);
  if (!ofstream_)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't open file="
           << file_path_;
    throw Exception(stream);
  }
}

void ArchiveOfstream::check()
{
  if (!good())
  {
    Stream::Error stream;
    stream << FNS
           << "ArchiveIfstream is failed";
    throw Exception(stream);
  }
}

} // namespace AdServer::LogProcessing