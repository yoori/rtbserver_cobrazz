#include <unistd.h>
#include <eh/Errno.hpp>
#include "ChunkUtils.hpp"

namespace
{
  const char LOG_RE_TIME_FORMAT[] = "(\\d{8}\\.\\d{6})(\\.\\d{8})?";
  const char LOG_TIME_FORMAT[] = "%Y%m%d.%H%M%S";
}

namespace AdServer
{
namespace UserInfoSvcs
{
  // BaseChunkSelector
  BaseChunkSelector::BaseChunkSelector(const String::SubString& prefix)
  {
    std::string re_str = prefix.str();
    re_str += "\\.";
    re_str += LOG_RE_TIME_FORMAT;
    reg_exp_.set_expression(re_str);
  }

  bool
  BaseChunkSelector::match(std::string* date_str, const char* full_path)
    const
    noexcept
  {
    String::RegEx::Result sub_strs;

    String::SubString file_name(
      Generics::DirSelect::file_name(full_path));

    if(reg_exp_.search(sub_strs, file_name) &&
      (assert(!sub_strs.empty()), sub_strs[0].length() == file_name.size()))
    {
      if(date_str)
      {
        assert(sub_strs.size() >= 2);
        *date_str = sub_strs[1].str();
      }

      return true;
    }

    return false;
  }

  // ChunkRemover
  ChunkRemover::ChunkRemover(const String::SubString& prefix)
    : BaseChunkSelector(prefix)
  {}

  bool
  ChunkRemover::operator ()(const char* full_path, const struct stat&)
    const
    /*throw(BaseChunkSelector::Exception)*/
  {
    if(match(0, full_path))
    {
      if(::unlink(full_path) == -1)
      {
        eh::throw_errno_exception<BaseChunkSelector::Exception>(
          "can't remove file '",
          full_path,
          "'");
      }

      return true;
    }

    return false;
  }

  // ChunkSelector
  ChunkSelector::ChunkSelector(
    const String::SubString& prefix,
    ChunkFileDescriptionMap& chunk_files)
    : BaseChunkSelector(prefix),
      chunk_files_(chunk_files)
  {}

  bool
  ChunkSelector::operator ()(const char* full_path, const struct stat&)
    /*throw(BaseChunkSelector::Exception)*/
  {
    std::string date_str;

    if(match(&date_str, full_path))
    {
      try
      {
        chunk_files_.insert(
          std::make_pair(
            full_path,
            ChunkFileDescription(
              Generics::Time(date_str, LOG_TIME_FORMAT))));
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "can't parse file name '" <<
          full_path << "': " << ex.what();
        throw BaseChunkSelector::Exception(ostr);
      }

      return true;
    }

    return false;
  }
}
}
