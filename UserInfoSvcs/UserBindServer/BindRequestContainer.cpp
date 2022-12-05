#include <iostream>
#include <fstream>
#include <sstream>
#include <Generics/Time.hpp>
#include <PrivacyFilter/Filter.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/Rand.hpp>
#include <String/RegEx.hpp>

#include <Commons/Algs.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/LogCommons.ipp>

#include "BindRequestContainer.hpp"

namespace Aspect
{
  const char BIND_REQUEST_CONTAINER[] = "BindRequestContainer";
}

namespace
{
  const char LOG_RE_TIME_FORMAT[] = "\\d{8}\\.\\d{6}";
  const char LOG_TIME_FORMAT[] = "%Y%m%d.%H%M%S";
}

namespace AdServer
{
namespace UserInfoSvcs
{
  struct BaseChunkSelector
  {
    BaseChunkSelector(const char* prefix)
    {
      std::string re_str = prefix;
      re_str += "\\.";
      re_str += std::string("(") + LOG_RE_TIME_FORMAT + ")";
      reg_exp_.set_expression(re_str);
    }

    bool
    match(std::string* date_str, const char* full_path)
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
          assert(sub_strs.size() == 2);
          *date_str = sub_strs[1].str();
        }

        return true;
      }

      return false;
    }

  private:
    String::RegEx reg_exp_;
  };

  struct ChunkRemover: public BaseChunkSelector
  {
    ChunkRemover(const char* prefix)
      : BaseChunkSelector(prefix)
    {}

    bool
    operator ()(const char* full_path, const struct stat&)
      const
      noexcept
    {
      if(match(0, full_path))
      {
        if(::unlink(full_path) == -1)
        {
          eh::throw_errno_exception<BindRequestContainer::Exception>(
            "can't remove file '",
            full_path,
            "'");
        }

        return true;
      }

      return false;
    }
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
      const char* prefix,
      ChunkFileDescriptionMap& chunk_files)
      : BaseChunkSelector(prefix),
        chunk_files_(chunk_files)
    {}

    bool
    operator ()(const char* full_path, const struct stat&)
      /*throw(BindRequestContainer::Exception)*/
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
          throw BindRequestContainer::Exception(ostr);
        }
        
        return true;
      }

      return false;
    }

  private:
    /// Reference to container that get result map of files
    ChunkFileDescriptionMap& chunk_files_;
  };

  // BindRequestContainer
  BindRequestContainer::BindRequestContainer(
    Logging::Logger* logger,
    unsigned long common_chunks_number,
    const ChunkPathMap& chunk_folders,
    const char* file_prefix,
    const Generics::Time& extend_time_period,
    unsigned long portions_number)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      common_chunks_number_(common_chunks_number)
  {
    chunks_.resize(common_chunks_number);

    // load chunks
    // TODO : concurrent loading - local taskrunner with destroy
    for(ChunkPathMap::const_iterator chunk_it = chunk_folders.begin();
        chunk_it != chunk_folders.end(); ++chunk_it)
    {
      std::cerr << "to load chunk #" << chunk_it->first << "/" << common_chunks_number_ << std::endl;
      chunks_[chunk_it->first] = new BindRequestChunk(
        logger,
        chunk_it->second.c_str(),
        file_prefix,
        extend_time_period,
        portions_number);
    }
  }

  BindRequestContainer::~BindRequestContainer() noexcept
  {}

  BindRequestContainer::BindRequest
  BindRequestContainer::get_bind_request(
    const String::SubString& id,
    const Generics::Time& now)
    /*throw(ChunkNotFound, Exception)*/
  {
    return get_chunk_(id)->get_bind_request(
      id,
      now);
  }

  void
  BindRequestContainer::add_bind_request(
    const String::SubString& id,
    const BindRequest& bind_request,
    const Generics::Time& now)
    /*throw(ChunkNotFound, Exception)*/
  {
    return get_chunk_(id)->add_bind_request(
      id,
      bind_request,
      now);
  }

  void
  BindRequestContainer::clear_expired(const Generics::Time& expire_time)
    /*throw(Exception)*/
  {
    for(ChunkArray::iterator chunk_it = chunks_.begin();
        chunk_it != chunks_.end(); ++chunk_it)
    {
      if(*chunk_it)
      {
        (*chunk_it)->clear_expired(expire_time);
      }
    }
  }

  void
  BindRequestContainer::dump() /*throw(Exception)*/
  {
    for(ChunkArray::iterator chunk_it = chunks_.begin();
        chunk_it != chunks_.end(); ++chunk_it)
    {
      if(*chunk_it)
      {
        (*chunk_it)->dump();
      }
    }
  }

  BindRequestChunk_var
  BindRequestContainer::get_chunk_(
    const String::SubString& external_id) const
    /*throw(ChunkNotFound)*/
  {
    BindRequestChunk_var res = chunks_[
      AdServer::Commons::external_id_distribution_hash(
        external_id) % common_chunks_number_];

    if(!res)
    {
      throw ChunkNotFound("");
    }

    return res;
  }
} /* namespace UserInfoSvcs */
} /* namespace AdServer */
