#include <list>
#include <String/AsciiStringManip.hpp>
#include <HTTP/UrlAddress.hpp>

#include "PathManip.hpp"

namespace AdServer
{
  namespace PathManip
  {
    bool
    relative_path(const char* from_path,
      const char* to_path,
      std::string& result)
      noexcept
    {
      result.clear();
      if (!*from_path || !*to_path)
      {
        return false;
      }
      // Recover relative path, need to jump from_path to to_path.
      // 1. Compare from_path with to_path
      // Equal parts reject.
      // 2. Relative = repeat {../} by the number of remaining '/'
      //  in the rest part of from_path.
      // 3. Relative += rest of to_path.

      // Calculate equal parts of path
      for (;
        *from_path == *to_path && *from_path;
        ++from_path, ++to_path)
      {
      }

      if (*from_path)
      {
        while (*++from_path)
        {
          if (*from_path == '/')
          {
            result += "../";
          }
        }
        if (*--from_path != '/')
        {
            result += "../";
        }
      }
      else
      { // "./" it is not necessary may be
        result += "./";
        if (*to_path == '/')
        {
          ++to_path;
        }
      }
      // append rest - informative part of received URL
      result.append(to_path);
      return true;
    }

    std::string
    uri_to_fpath(const String::SubString& uri)
      noexcept
    {
      try
      {
        HTTP::URLAddress address(uri);
        if (address.scheme() == String::AsciiStringManip::Caseless("file"))
        {
          return address.path().str();
        }
      }
      catch (const eh::Exception&)
      {
      }
      return std::string();
    }

    void
    create_path(std::string& first, const char* second)
      /*throw(eh::Exception)*/
    {
      if(second && second[0] == '/')
      {
        first = second;
        return;
      }
      if(*(first.rbegin()) != '/')
      {
        first += "/";
      }
      first += second;
    }

    bool
    normalize_path(std::string& path) noexcept
    {
      typedef std::list<String::SubString> PartList;

      if(path.empty() || path == "/")
      {
        return true;
      }

      size_t end_pos = path.size() - (*path.rbegin() == '/' ? 1 : 0);
      size_t start_pos = 0;
      unsigned long skip_parts = 0;
      PartList result_parts;
      size_t res_reserve_size = 0;

      while(end_pos != 0)
      {
        start_pos = path.rfind('/', end_pos - 1);
        size_t part_start_pos = start_pos + 1;

        if(start_pos == std::string::npos)
        {
          part_start_pos = start_pos = 0;
        }

        if(end_pos - part_start_pos != 0 &&
           (end_pos - part_start_pos != 1 || path[part_start_pos] != '.'))
           // ignore '//', '/./' appearance
        {
          if(end_pos - part_start_pos == 2 &&
             path[part_start_pos] == '.' && path[part_start_pos + 1] == '.')
          {
            ++skip_parts;
          }
          else if(skip_parts)
          {
            --skip_parts;
          }
          else
          {
            result_parts.push_front(String::SubString(
              path.c_str() + start_pos,
              end_pos - start_pos));
            res_reserve_size += end_pos - start_pos;
          }
        }

        end_pos = start_pos;
      }
      
      if(skip_parts != 0)
      {
        return false;
      }

      // construct result
      std::string result;
      result.reserve(res_reserve_size);

      for(PartList::const_iterator part_it =
            result_parts.begin();
          part_it != result_parts.end(); ++part_it)
      {
        result.append(part_it->data(), part_it->size());
      }

      if(result.empty() && path[0] == '/')
      {
        result = "/";
      }

      path.swap(result);

      return true;
    }

    void
    split_path(
      const char* path_name,
      std::string* path,
      std::string* name,
      bool strip_slash)
      noexcept
    {
      std::string path_pattern_s(path_name);
      std::string::size_type pos = path_pattern_s.find_last_of('/');
      if (pos != std::string::npos && pos != path_pattern_s.size() - 1)
      {
        if(path)
        {
          path->assign(path_pattern_s, 0, strip_slash ? pos : pos +  1);
        }
        if(name)
        {
          name->assign(
            path_pattern_s,
            pos + 1,
            path_pattern_s.size() - pos - 1);
        }
      }
      else
      {
        if(path)
        {
          path->swap(path_pattern_s);
        }
        if(name)
        {
          name->clear();
        }
      }
    }

  }
}
