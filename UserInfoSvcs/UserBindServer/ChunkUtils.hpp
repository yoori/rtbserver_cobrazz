#ifndef USERINFOSVCS_USERBINDSERVER_CHUNKUTILS_HPP
#define USERINFOSVCS_USERBINDSERVER_CHUNKUTILS_HPP

#include <iostream>
#include <fstream>
#include <sstream>

#include <Generics/Time.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/Rand.hpp>
#include <String/RegEx.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  // path to file name map
  typedef std::map<std::string, std::string> FilePathMap;

  struct BaseChunkSelector
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    BaseChunkSelector(const String::SubString& prefix);

    bool
    match(std::string* date_str, const char* full_path)
      const
      noexcept;

  private:
    String::RegEx reg_exp_;
  };

  struct ChunkRemover: public BaseChunkSelector
  {
    ChunkRemover(const String::SubString& prefix);

    bool
    operator ()(const char* full_path, const struct stat&)
      const
      /*throw(BaseChunkSelector::Exception)*/;
  };

  struct ChunkSelector: public BaseChunkSelector
  {
    struct ChunkFileDescription
    {
      ChunkFileDescription(
        const Generics::Time& max_keep_time)
        : max_time(max_keep_time)
      {}

      Generics::Time max_time;
    };

    /// In really used full_path -> max_keep_time map.
    typedef std::map<std::string, ChunkFileDescription>
      ChunkFileDescriptionMap;

    ChunkSelector(
      const String::SubString& prefix,
      ChunkFileDescriptionMap& chunk_files);

    bool
    operator ()(const char* full_path, const struct stat&)
      /*throw(BaseChunkSelector::Exception)*/;

  private:
    /// Reference to container that get result map of files
    ChunkFileDescriptionMap& chunk_files_;
  };
}
}

#endif /*USERINFOSVCS_USERBINDSERVER_CHUNKUTILS_HPP*/
