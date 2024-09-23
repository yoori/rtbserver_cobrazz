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
  const std::streamsize buffer_size,
  const WriterType writer_type) noexcept
{
  const bool ok = init(
    file_path,
    params,
    mode,
    buffer_size,
    writer_type);
  if (!ok)
  {
    setstate(std::ios_base::failbit);
  }
}

ArchiveOfstream::ArchiveOfstream(
  const std::string& file_path,
  const Bzip2Params& params,
  const std::ios_base::openmode mode,
  const std::streamsize buffer_size,
  const WriterType writer_type) noexcept
{
  const bool ok = init(
    file_path,
    params,
    mode,
    buffer_size,
    writer_type);
  if (!ok)
  {
    setstate(std::ios_base::failbit);
  }
}

ArchiveOfstream::ArchiveOfstream(
  const std::string& file_path,
  const ArchiveParams& params,
  const std::ios_base::openmode mode,
  const std::streamsize buffer_size,
  const WriterType writer_type) noexcept
{
  if (const auto* pval = std::get_if<GzipParams>(&params))
  {
    const bool ok = init(
      file_path,
      *pval,
      mode,
      buffer_size,
      writer_type);
    if (!ok)
    {
      setstate(std::ios_base::failbit);
    }
  }
  else if (const auto* pval = std::get_if<Bzip2Params>(&params))
  {
    const bool ok = init(
      file_path,
      *pval,
      mode,
      buffer_size,
      writer_type);
    if (!ok)
    {
      setstate(std::ios_base::failbit);
    }
  }
  else
  {
    setstate(std::ios_base::failbit);
  }
}

ArchiveOfstream::~ArchiveOfstream()
{
  try
  {
    flush();
    reset();
  }
  catch (...)
  {
  }
}

const std::string& ArchiveOfstream::file_path() const noexcept
{
  return file_path_;
}

const std::string& ArchiveOfstream::file_extension() const noexcept
{
  return file_extension_;
}

bool ArchiveOfstream::init(
  const std::string& file_path,
  const GzipParams& params,
  const std::ios_base::openmode mode,
  const std::streamsize buffer_size,
  const WriterType writer_type) noexcept
{
  const bool ok = open_file(
    file_path,
    GZIP_EXTENSION,
    mode,
    writer_type);
  if (!ok)
  {
    return false;
  }

  try
  {
    GzipParams result_params = params;
    result_params.noheader = false;

    boost::iostreams::gzip_compressor compressor{result_params, buffer_size};
    boost::iostreams::filtering_ostream::push(compressor);

    if (writer_type == WriterType::Ofstream)
    {
      boost::iostreams::filtering_ostream::push(*ofstream_);
    }
    else
    {
      boost::iostreams::filtering_ostream::push(*file_descriptor_sink_);
    }

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool ArchiveOfstream::init(
  const std::string& file_path,
  const Bzip2Params& params,
  const std::ios_base::openmode mode,
  const std::streamsize buffer_size,
  const WriterType writer_type) noexcept
{
  const bool ok = open_file(
    file_path,
    BZ2_EXTENSION,
    mode,
    writer_type);
  if (!ok)
  {
    return false;
  }

  try
  {
    boost::iostreams::bzip2_compressor compressor{params, buffer_size};
    boost::iostreams::filtering_ostream::push(compressor);

    if (writer_type == WriterType::Ofstream)
    {
      boost::iostreams::filtering_ostream::push(*ofstream_);
    }
    else
    {
      boost::iostreams::filtering_ostream::push(*file_descriptor_sink_);
    }

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool ArchiveOfstream::open_file(
  const std::string& file_path,
  const std::string_view extension,
  const std::ios_base::openmode mode,
  const WriterType writer_type) noexcept
{
  try
  {
    file_extension_ = extension;
    file_path_ = file_path + file_extension_;

    if (writer_type == WriterType::Ofstream)
    {
      ofstream_ = std::make_unique<std::ofstream>(
        file_path_,
        mode | std::ios_base::out);
      if (!ofstream_->good())
      {
        return false;
      }
    }
    else
    {
      file_descriptor_sink_ = std::make_unique<FileDescriptorSink>(
        file_path_,
        mode | std::ios_base::out);
      if (!file_descriptor_sink_->is_open())
      {
        return false;
      }
    }

    return true;
  }
  catch (...)
  {
  }

  return false;
}

} // namespace AdServer::LogProcessing