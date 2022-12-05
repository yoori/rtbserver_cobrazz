
#ifndef HOST_DISTRIBUTION_FILE_HPP_
#define HOST_DISTRIBUTION_FILE_HPP_

#include <string>
#include <map>
#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Stream/MemoryStream.hpp>

namespace AdServer
{
namespace Commons
{

  /**
   * Class for finding host by index, defined in configuration file
   * */
  class HostDistributionFile: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(InvalidFile, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidIndex, InvalidFile);
    DECLARE_EXCEPTION(FileNotFound, InvalidFile);

    typedef std::map<unsigned long, std::string> IndexToHostMap;

    /**
     * @param file configuration file
     * */ 
    HostDistributionFile(const char* file, const char* schema_file)
      /*throw(FileNotFound, InvalidFile)*/;

    /** 
     * Returns hostname by index value
     * @param index
     * @return host name for index
     * */
    const std::string& get_host_by_index(unsigned long index) const
      /*throw(InvalidIndex)*/;

    /** 
     * Returns maximum index
     * @return maximum index + 1
     * */
    unsigned long get_index_limit() const noexcept;

    /** 
     * Returns Map of indexes to host
     * @return Map of indexes
     * */
    const IndexToHostMap& all_indexes() const;

  protected:
    virtual ~HostDistributionFile() noexcept
    {}

  private:
    IndexToHostMap host_map_;
  };

  typedef ReferenceCounting::SmartPtr<HostDistributionFile>
    HostDistributionFile_var;
}
}

namespace AdServer
{
namespace Commons
{
  inline
  const std::string& HostDistributionFile::get_host_by_index(
    unsigned long index) const
    /*throw(InvalidIndex)*/
  {
    IndexToHostMap::const_iterator fnd = host_map_.find(index);
    if(fnd == host_map_.end())
    {
      Stream::Error err;
      err << "Invalid index " << index;
      throw InvalidIndex(err);
    }
    return fnd->second;
  }

  inline
  unsigned long HostDistributionFile::get_index_limit() const noexcept
  {
    return host_map_.rbegin()->first + 1;
  }

  inline
  const HostDistributionFile::IndexToHostMap&
  HostDistributionFile::all_indexes() const
  {
    return host_map_;
  }

}
}

#endif
