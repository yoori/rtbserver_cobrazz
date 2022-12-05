#include <cstring>
#include "Namespace.hpp"

namespace Declaration
{
  NamePath::NamePath(const char* abs_path, bool name_is_local)
    /*throw(InvalidName)*/
  {
    /*
    if(!*abs_path)
    {
      throw InvalidName("");
    }
    */

    const char* path_part_begin = abs_path;

    if(!name_is_local)
    {
      const char* path_part_end = 0;
      while((path_part_end = ::strchr(path_part_begin, ':')))
      {
        push_back(std::string(path_part_begin, path_part_end));
        path_part_begin = path_part_end;
        ++path_part_begin;
        if(!*path_part_begin || *path_part_begin != ':')
        {
          throw InvalidName("");
        }
        ++path_part_begin;
      }
    }

    push_back(path_part_begin);
  }

  std::string
  NamePath::str() const noexcept
  {
    std::string ret;
    for(const_iterator it = begin(); it != end(); ++it)
    {
      if(it != begin())
      {
        ret += "::";
      }

      ret += *it;
    }

    return ret;
  }
  
  Namespace::Namespace(
    const char* name_val,
    Namespace* owner_val)
    noexcept
    : name_(name_val ? name_val : ""),
      owner_(owner_val)
  {}

  const char*
  Namespace::name() const noexcept
  {
    return name_.c_str();
  }

  NamePath
  Namespace::abs_name() const noexcept
  {
    NamePath ret(name(), true);
    Namespace_var cur = owner();
    while(cur.in())
    {
      if(cur->name()[0] != 0) // ignore unnamed namespace
      {
        ret.push_front(cur->name());
      }
      cur = cur->owner();
    }

    return ret;
  }

  Namespace_var
  Namespace::owner() const noexcept
  {
    return ReferenceCounting::add_ref(owner_);
  }

  const Namespace::BaseTypeMap&
  Namespace::types() const noexcept
  {
    return types_;
  }

  const Namespace::NamespaceMap&
  Namespace::namespaces() const noexcept
  {
    return namespaces_;
  }

  BaseType_var
  Namespace::find_type(const NamePath& name_path) const
    noexcept
  {
    const Namespace* search_ns = this;
    while(search_ns)
    {
      BaseType_var ret = search_ns->local_find_type_(name_path);
      if(ret.in())
      {
        return ret;
      }
      search_ns = search_ns->owner_;
    }
    
    return BaseType_var();
  }

  BaseType_var
  Namespace::find_local_type(const char* name) const
    noexcept
  {
    BaseTypeMap::const_iterator type_it = types().find(name);
    if(type_it != types().end())
    {
      return type_it->second;
    }
    
    return BaseType_var();
  }

  Namespace_var
  Namespace::add_namespace(const char* name_val)
    noexcept
  {
    NamespaceMap::const_iterator ns_it = namespaces_.find(name_val);
    if(ns_it != namespaces_.end())
    {
      return ns_it->second;
    }
    
    Namespace_var new_ns(new Namespace(name_val, this));
    namespaces_.insert(std::make_pair(name_val, new_ns));
    return new_ns;
  }

  void
  Namespace::add_type(BaseType* new_type)
    /*throw(AlreadyDefined)*/
  {
    types_.insert(std::make_pair(
      new_type->name(),
      ReferenceCounting::add_ref(new_type)));
  }

  BaseType_var
  Namespace::local_find_type_(const NamePath& name_path) const
    noexcept
  {
    NamePath::const_iterator np_it = name_path.begin();
    const Namespace* search_ns = this;
    while(np_it != --name_path.end())
    {
      NamespaceMap::const_iterator ns_it = search_ns->namespaces().find(*np_it);
      if(ns_it == search_ns->namespaces().end())
      {
        return BaseType_var();
      }
      search_ns = ns_it->second;
      ++np_it;
    }

    BaseTypeMap::const_iterator type_it = search_ns->types().find(*np_it);
    if(type_it != search_ns->types().end())
    {
      return type_it->second;
    }

    return BaseType_var();
  }
}

