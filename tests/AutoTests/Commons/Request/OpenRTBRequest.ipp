
namespace AutoTest
{
  // OpenRTBRequest::Param
  
  template <OpenRtb::TagConst Tag, OpenRtb::EscapeJSON escape>
  template <class T>
  OpenRTBRequest::Param<Tag, escape>::Param(
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const char* name,
    const T& defs,
    bool set_defs) :
    Parameter(group, name, Tag, escape),
    request_(request)
  {
    if (set_defs)
    {
      Parameter::set_param_val(defs);
    }
  }
  
  template <OpenRtb::TagConst Tag, OpenRtb::EscapeJSON escape>
  OpenRTBRequest::Param<Tag, escape>::Param(
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const Param& other) :
    Parameter(group, other),
    request_(request)
  { }

  template <OpenRtb::TagConst Tag, OpenRtb::EscapeJSON escape>
  OpenRTBRequest::Param<Tag, escape>::Param(
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const char* name) :
    Parameter(group, name, Tag, escape),
    request_(request)
  { }

  template <OpenRtb::TagConst Tag, OpenRtb::EscapeJSON escape>
  OpenRTBRequest::Param<Tag, escape>::~Param() noexcept
  { }

  template <OpenRtb::TagConst Tag, OpenRtb::EscapeJSON escape>
  template <class T>
  OpenRTBRequest::Param<Tag, escape>&
  OpenRTBRequest::Param<Tag, escape>::operator= (
    const T& val)
  {
    Parameter::set_param_val(val);
    return *this;
  }

  template <OpenRtb::TagConst Tag, OpenRtb::EscapeJSON escape>
  template <class T>
  OpenRTBRequest&
  OpenRTBRequest::Param<Tag, escape>::operator() (
    const T& val)
  {
    Parameter::set_param_val(val);
    return static_cast<OpenRTBRequest&>(*request_);
  }

  // ParamsGenerator
  
  template<class ParamType>
  template<typename... Args>
  ParamType* 
  OpenRTBRequest::ParamsGenerator<ParamType>::operator()(
    BaseParamsContainer* request,
    Args&&... args)
  {
    return new ParamType(
      static_cast<OpenRTBRequest*>(request),
      std::forward<Args>(args)...);
  }

  // OpenRTBRequest::ParamsArray

  template<class ParamType>
  template <class T>
  OpenRTBRequest::ParamsArray<ParamType>::ParamsArray(
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const char* name,
    const T& defs,
    unsigned short flags) :
    Base(request, group, name),
    current_index_(0)
  {
    if (flags & OpenRtb::RF_SET_DEFS)
    {
      Base::set_param_val(defs, current_index_++);
    }
  }

  template<class ParamType>
  OpenRTBRequest::ParamsArray<ParamType>::ParamsArray(
    size_t size,
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const char* name,
    unsigned short /*flags*/) :
    Base(request, group, name),
    current_index_(size)
  {
    for (size_t i = 0; i < size; i++)
    {
      Base::parameters_[i] =
        ParamsGenerator<ParamType>()(
          this->request_, this, strof(i).c_str());
    }
  }

  template<class ParamType>
  OpenRTBRequest::ParamsArray<ParamType>::ParamsArray(
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const char* name) :
    Base(request, group, name),
    current_index_(0)
  { }

  template<class ParamType>
  OpenRTBRequest::ParamsArray<ParamType>::ParamsArray(
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const ParamsArray& other):
    Base(request, group, other.name_.c_str()),
    current_index_(other.current_index_)
  {
    for (auto it = other.Base::parameters_.begin();
         it != other.Base::parameters_.end(); ++it)
    {
      Base::parameters_[it->first] =
        ParamsGenerator<ParamType>()(
          this->request_, this, *it->second);
    }
  }

  template<class ParamType>
  template <class T>
  OpenRTBRequest::ParamsArray<ParamType>&
  OpenRTBRequest::ParamsArray<ParamType>::operator= (
    const T& val)
  {
    this->set_param_val(val, current_index_++);
    return *this;
  }

  template<class ParamType>
  template <class T>
  OpenRTBRequest&
  OpenRTBRequest::ParamsArray<ParamType>::operator() (
    const T& val,
    size_t  group_id)
  {
    Base::operator()(val, group_id);
    if (group_id >= current_index_)
    { current_index_ = group_id + 1; }
    return static_cast<OpenRTBRequest&>(*Base::request_);
  }

  template<class ParamType>
  template <class T>
  OpenRTBRequest&
  OpenRTBRequest::ParamsArray<ParamType>::operator() (
    const T& val)
  {
    return operator()(val, current_index_++);
  }

  template<class ParamType>
  bool
  OpenRTBRequest::ParamsArray<ParamType>::empty() const
  {
    return Base::empty();
  }

  template<class ParamType>
  bool
  OpenRTBRequest::ParamsArray<ParamType>::print(
    std::ostream&,
    const char*,
    const char*) const
  {
    return false;
  }

  template<class ParamType>
  bool
  OpenRTBRequest::ParamsArray<ParamType>::clear(
    unsigned int index)
  {
    if (Base::clear(index))
    {
      if (!empty())
      {
        current_index_ = Base::parameters_.rbegin()->first + 1;
      }
      else
      {
        current_index_ = 0;
      }
      return true;
    }
    return false;
  }

  template<class ParamType>
  void
  OpenRTBRequest::ParamsArray<ParamType>::clear()
  {
    clear(--current_index_);
  }

  template<class ParamType>
  void
  OpenRTBRequest::ParamsArray<ParamType>::set_param_val(
    const String::SubString& val)
  {
    Base::set_param_val(val, current_index_++);
  }

  template <class ParamType>
  void
  OpenRTBRequest::ParamsArray<ParamType>::print(
    std::ostream& out, 
    unsigned long indent,
    bool print_name) const
  {
    if (!empty())
    {
      if (print_name)
      {
        out << std::string(indent, ' ') << "\"" << Base::name_ << "\" : ";
      }

      out << OpenRtb::ARRAY_TAG.begin;
      for(auto it = Base::parameters_.begin() ; it != Base::parameters_.end(); ++it)
      {
        if (it != Base::parameters_.begin())
        {
          out << ", ";
        }
        it->second->print(out, indent+2, false);
      }

      out << OpenRtb::ARRAY_TAG.end;
    }
  }

  // OpenRTBRequest::ProxyParam

  template <class ParamType>
  OpenRTBRequest::ProxyParam<ParamType>::ProxyParam(
    OpenRTBRequest* request,
    ParamType& param) :
    BaseParam(request, ""),
    param_(param)
  { }

  template <class ParamType>
  OpenRTBRequest::ProxyParam<ParamType>::~ProxyParam() noexcept
  { }

  template <class ParamType>
  void
  OpenRTBRequest::ProxyParam<ParamType>::set_param_val(
    const String::SubString& val)
  {
    param_.set_param_val(val);
  }

  template <class ParamType>
  void
  OpenRTBRequest::ProxyParam<ParamType>::clear()
  {
    param_.clear();
  }

  template <class ParamType>
  bool
  OpenRTBRequest::ProxyParam<ParamType>::empty () const
  {
    return param_.empty();
  }

  template <class ParamType>
  std::string
  OpenRTBRequest::ProxyParam<ParamType>::str() const
  {
    return param_.str();
  }

  template <class ParamType>
  std::string
  OpenRTBRequest::ProxyParam<ParamType>::raw_str() const
  {
    return param_.raw_str();
  }

  template <class ParamType>
  bool
  OpenRTBRequest::ProxyParam<ParamType>::print (
    std::ostream&,
    const char*,
    const char*) const
  {
    return false;
  }

  template <class ParamType>
  template <class T>
  ParamType&
  OpenRTBRequest::ProxyParam<ParamType>::operator=(
    const T& val)
  {
    return param_.operator=(val);
  }

  template <class ParamType>
  template <class T>
  OpenRTBRequest&
  OpenRTBRequest::ProxyParam<ParamType>::operator() (const T& val)
  {
    return param_.operator()(val);
  }
}
