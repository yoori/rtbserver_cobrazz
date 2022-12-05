#ifndef _NAMESPACE_HPP_
#define _NAMESPACE_HPP_

#include <list>
#include <map>
#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "BaseType.hpp"

namespace Declaration
{
  class NamePath: public std::list<std::string>
  {
  public:
    DECLARE_EXCEPTION(InvalidName, eh::DescriptiveException);
    
    NamePath(const char* abs_path, bool name_is_local = false)
      /*throw(InvalidName)*/;

    std::string str() const noexcept;
  };

  class Namespace;

  typedef ReferenceCounting::SmartPtr<Namespace> Namespace_var;

  class Namespace: public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(AlreadyDefined, eh::DescriptiveException);

    typedef std::map<std::string, BaseType_var>
      BaseTypeMap;

    typedef std::map<std::string,
      ReferenceCounting::SmartPtr<Namespace> >
      NamespaceMap;
    
  public:
    Namespace(
      const char* name_val = 0,
      Namespace* owner_val = 0)
      noexcept;

    const char* name() const noexcept;

    NamePath abs_name() const noexcept;

    Namespace_var owner() const noexcept;

    const BaseTypeMap& types() const noexcept;

    const NamespaceMap& namespaces() const noexcept;

    BaseType_var find_type(const NamePath& name) const noexcept;

    BaseType_var find_local_type(const char* name) const noexcept;

    ReferenceCounting::SmartPtr<Namespace>
    add_namespace(const char* name) noexcept;

    void add_type(BaseType*) /*throw(AlreadyDefined)*/;

  private:
    virtual ~Namespace() noexcept {}
    
    BaseType_var local_find_type_(const NamePath& name) const noexcept;
    
  private:
    std::string name_;
    Namespace* owner_;
    BaseTypeMap types_;
    NamespaceMap namespaces_;
  };

  typedef ReferenceCounting::SmartPtr<Namespace>
    Namespace_var;
}

#endif /*_NAMESPACE_HPP_*/
