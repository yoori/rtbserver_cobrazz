// @file PathManip.hpp

#ifndef PATH_MANIP_INCLUDED
#define PATH_MANIP_INCLUDED

#include <string>
#include <eh/Exception.hpp>
#include <String/SubString.hpp>

namespace AdServer
{
  namespace PathManip
  {
    /**
     * Compute relative path from_path -> to_path
     * @param from_path Pure absolute path, not URI or URL, without scheme.
     * Suppose that always have slash at end, even nothing.
     * @param to_path Pure absolute path to original file,
     * can contain file name
     * @param result Computed result of function
     *
     * @return true if successfully calculated relative path,
     * false and set result to empty string, if cannot.
     */
    bool
    relative_path(const char* from_path,
      const char* to_path,
      std::string& result)
      noexcept;

    /**
     * Get uri, check that it file URL, if true, extract path
     * @param uri Full resolved absolute uri, MUST contain scheme.
     * @return The string that contain path, if string empty, path is
     * not to file
     */
    std::string
    uri_to_fpath(const String::SubString& uri) noexcept;

    /**
     * Concatenate root and path or use root path
     * @param first log root and result.
     * @param second part of path.
     * @return none
     */
    void
    create_path(std::string& first, const char* second)
      /*throw(eh::Exception)*/;

    /**
     * Remove usage of '/./', '/../' and extra '/' from path,
     * Difference with ::realpath :
     *   if ../ can't be removed (relative path that reference to up directory)
     *     will be returned false,
     *   don't resolve symbolic links and don't check file existence
     * @param path file path to normalize and result
     */
    bool normalize_path(std::string& path) noexcept;

    /**
     * Split path name on path and file name,
     * if path or file name is 0, it is ignored
     * @param path name
     * @param pointer on path
     * @param pointer on file name
     * @param strip last slash in path
     * @return none
     */
    void
    split_path(
      const char* path_name,
      std::string* path,
      std::string* name,
      bool strip_slash = true)
      noexcept;
  }
}

#endif // PATH_MANIP_INCLUDED
