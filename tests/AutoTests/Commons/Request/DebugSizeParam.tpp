
namespace AutoTest
{
  template<typename Request>
  template<class T>
  DebugSizeParam<Request>::DebugSizeParam(
    Request* request,
    const char* name,
    const T& defs,
    bool set_defs): Base(request, name, defs, set_defs)
  {}

  template<typename Request>
  template<class T>
  DebugSizeParam<Request>&
  DebugSizeParam<Request>::operator= (
    const T& val)
  {
    Base::operator=(val);
    return *this;
  }

  template<typename Request>
  DebugSizeParam<Request>::DebugSizeParam(
    Request* request,
    const char* name) : Base(request, name)
  {}

  template<typename Request>
  DebugSizeParam<Request>::DebugSizeParam(
    Request* request,
    const DebugSizeParam& other):
    Base(request, static_cast<const Base&>(other))
  {}

  template<typename Request>
  DebugSizeParam<Request>::~DebugSizeParam() noexcept
  {}

  template<typename Request>
  bool DebugSizeParam<Request>::print(
    std::ostream& out,
    const char* prefix,
    const char* eql) const
  {
    out << prefix;
    if (!Base::param_value_.empty())
    {
      out << Base::name_ << eql << Base::str();
    }

    typename Base::ParamsMap::const_iterator it = Base::parameters_.begin();
    for(;it != Base::parameters_.end(); ++it)
    {
      if (it != Base::parameters_.begin() || !Base::param_value_.empty())
      {
        out << static_cast<Request*>(Base::request_)->params_prefix();
      }

      out << "debug.adslot" << it->first << ".size" << eql << it->second->str();
    }
    return true;
  }
}

