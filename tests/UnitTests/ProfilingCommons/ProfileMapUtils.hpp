#ifndef PROFILEMAPUTILS_HPP
#define PROFILEMAPUTILS_HPP

#include <ProfilingCommons/ProfileMap/ExpireProfileMap.hpp>

template<typename MapType>
ReferenceCounting::SmartPtr<
  AdServer::ProfilingCommons::ProfileMap<typename MapType::KeyTypeT> >
init_map(
  const char* root,
  const char* sub_folder,
  const Generics::Time& extend_time,
  bool remove = true,
  ReferenceCounting::SmartPtr<MapType>* sub_map = 0)
  /*throw(eh::Exception)*/
{
  if(remove)
  {
    ::system((std::string("rm -r ") + root +
      sub_folder + " 2>/dev/null; mkdir -p " + root +
      sub_folder + "/").c_str());
  }

  ReferenceCounting::SmartPtr<MapType> map(
    new MapType((std::string(root) + sub_folder + "/").c_str(),
    "Chunk_",
    extend_time));

  if(sub_map)
  {
    *sub_map = map;
  }

  return map;
}

#endif /*PROFILEMAPUTILS_HPP*/
