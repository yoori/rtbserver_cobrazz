// STD
#include <filesystem>

// UNIXCOMMONS
#include <Generics/Function.hpp>
#include <Stream/MemoryStream.hpp>

// THIS
#include <LogCommons/ArchiveIfstream.hpp>

namespace AdServer::LogProcessing
{

ArchiveIfstream::ArchiveIfstream(
  const std::string& file_path,
  const std::ios_base::openmode mode,
  const DecompressorParams& params)
{
  const std::string extension = std::filesystem::path(
    file_path).extension();
  ifstream_.open(file_path, mode);
  if (!ifstream_)
  {
    setstate(std::ios_base::failbit);
    return;
  }

  if (extension == GZIP_EXTENSION)
  {
    const auto& gzip_decompressor_params = params.gzip_decompressor_params;
    boost::iostreams::gzip_decompressor decompressor(
      gzip_decompressor_params.gzip_window_bits,
      gzip_decompressor_params.buffer_size);
    boost::iostreams::filtering_istream::push(decompressor);
  }
  else if (extension == BZ2_EXTENSION)
  {
    const auto& bz2_decompressor_params = params.bz2_decompressor_params;
    boost::iostreams::bzip2_decompressor decompressor(
      bz2_decompressor_params.bz2_small,
      bz2_decompressor_params.buffer_size);
    boost::iostreams::filtering_istream::push(decompressor);
  }

  boost::iostreams::filtering_istream::push(ifstream_);
}

ArchiveIfstream::~ArchiveIfstream()
{
  reset();
}

bool ArchiveIfstream::is_archive(const std::string& file_path)
{
  const auto extension = std::filesystem::path(file_path).extension().string();
  return extension == GZIP_EXTENSION || extension == BZ2_EXTENSION;
}

} // namespace AdServer::LogProcessing