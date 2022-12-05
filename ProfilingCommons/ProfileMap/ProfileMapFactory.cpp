#include "ProfileMapFactory.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  class ChunkDirectoryAssigner
  {
  public:
    ChunkDirectoryAssigner(
      ProfileMapFactory::ChunkPathMap& chunks,
      const char* chunks_root,
      const char* chunks_prefix)
      noexcept
      : chunks_(chunks),
        chunks_root_(chunks_root),
        chunks_prefix_(chunks_prefix)
    {}

    bool operator()(const char* file_path, const struct stat& /*st*/)
      /*throw(eh::Exception)*/
    {
      String::SubString prefix;
      String::SubString chunk_index_str;
      String::SubString file_name_substr(file_path);

      unsigned long chunk_index;

      String::StringManip::Splitter<
        String::AsciiStringManip::SepUnderscore> tokenizer(
          file_name_substr);

      if(tokenizer.get_token(prefix) &&
         tokenizer.get_token(chunk_index_str) &&
         (prefix == chunks_prefix_) &&
         String::StringManip::str_to_int(chunk_index_str, chunk_index))
      {
        chunks_.insert(std::make_pair(chunk_index, chunks_root_ + "/" + file_path));
      }

      return true;
    }

  private:
    ProfileMapFactory::ChunkPathMap& chunks_;
    std::string chunks_root_;
    std::string chunks_prefix_;
  };

  void
  ProfileMapFactory::fetch_chunk_folders(
    ChunkPathMap& chunks,
    const char* chunks_root,
    const char* chunks_prefix)
    /*throw(eh::Exception)*/
  {
    ChunkDirectoryAssigner chunk_dir_assigner(chunks, chunks_root, chunks_prefix);

    Generics::DirSelect::directory_selector(
      chunks_root,
      Generics::DirSelect::wrap_functor(chunk_dir_assigner, true));
  }
}
}
