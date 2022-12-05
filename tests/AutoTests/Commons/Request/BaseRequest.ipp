namespace AutoTest
{
  // BaseParam

  inline
  BaseParam::BaseParam(
    BaseParamsContainer* request,
    const char* name) :
    request_(request),
    name_(name)
  {
    request_->add_param(this);
  }

  inline
  BaseParam::BaseParam(
    BaseParamsContainer* request,
    const BaseParam& other):
    request_(request),
    name_(other.name_)
  {
    request_->add_param(this);
  }
  
  // StringParam
  
  inline
  StringParam::StringParam(
    BaseParamsContainer* request,
    const char* name):
    BaseParam(request, name),
    empty_(true),
    need_encode_(true)
  { }

  inline
  StringParam::StringParam(
    BaseParamsContainer* request,
    const StringParam& other) :
    BaseParam(request, other),
    empty_(other.empty_),
    need_encode_(other.need_encode_)
  {
    if (!empty_)
    {       
      set_param_val(other.raw_str());
    }
  }

  template <class T>
  void StringParam::set_param_val(const T& val)
  {
    std::stringstream ostr;
    ostr << val;
    set_param_val(String::SubString(ostr.str()));
  }

  template <class T>
  StringParam&
  StringParam::operator=(const T& val)
  {
    set_param_val(val);
    return *this;
  }

  // SearchParam
  
  inline
  SearchParam::SearchParam(
    BaseParamsContainer* request,
    const char* name)
    : StringParam(request, name)
  { }

  inline
  SearchParam::SearchParam(
    BaseParamsContainer* request,
    const SearchParam& other)
    : StringParam(request, other)
  {
    set_param_val(other.raw_str());
  }
  
  template <class T>
  void SearchParam::set_param_val(const T& val)
  {
    std::stringstream ostr;
    ostr << val;
    set_param_val(String::SubString(ostr.str()));
  }

  // TimeParamBase

  template<typename TimeType>
  TimeParamBase<TimeType>::TimeParamBase(
    BaseParamsContainer* request,
    const char* name)
    : BaseParam(request, name), empty_(true)
  {
  }

  template<typename TimeType>
  TimeParamBase<TimeType>::TimeParamBase(
    BaseParamsContainer* request,
    const TimeParamBase& other)
    : BaseParam(request, other), empty_(other.empty_)
  {
    if (!empty_)
    {
      set_param_val(other.raw_str());
    }
  }

  template<typename TimeType>
  TimeParamBase<TimeType>::~TimeParamBase() noexcept
  { }

  template<typename TimeType>
  void
  TimeParamBase<TimeType>::clear()
  {
    empty_ = true;
  }

  template<typename TimeType>
  bool
  TimeParamBase<TimeType>::empty () const
  {
    return empty_;
  }

  template<typename TimeType>
  std::string
  TimeParamBase<TimeType>::str() const
  {
    if (request_->need_encode())
    {
      std::string ret;
      String::StringManip::mime_url_encode(
        get_gm_time().format(TimeType::format()), ret);
      return ret;
    }
    return get_gm_time().format(TimeType::format());
  }

  template<typename TimeType>
  std::string
  TimeParamBase<TimeType>::raw_str() const
  {
    return empty_? std::string():
      get_gm_time().format(TimeType::format());
  }

  template<typename TimeType>
  void
  TimeParamBase<TimeType>::set_param_val(
    const String::SubString& val)
  {
    static_cast<Time&>(*this) = Time(val);
    empty_ = false;
  }

  template<typename TimeType>
  void
  TimeParamBase<TimeType>::set_param_val(
    const std::string& val)
  {
    empty_ = val.empty();
    if (!val.empty())
    {
      set_param_val(Time(val));
    }
  }

  template<typename TimeType>
  void
  TimeParamBase<TimeType>::set_param_val(
    const char* val)
  {
    if (val)
    {
      set_param_val(Time(val));
    }
  }
  
  template<typename TimeType>
  void
  TimeParamBase<TimeType>::set_param_val(
    const time_t& val)
  {
    set_param_val(Time(val));
  }

  template<typename TimeType>
  void
  TimeParamBase<TimeType>::set_param_val(
    const Generics::Time& val)
  {
    static_cast<Time&>(*this) = val;
    empty_ = false;
  }

  template<typename TimeType>
  void
  TimeParamBase<TimeType>::set_param_val(
    const Time& val)
  {
    static_cast<Time&>(*this) = val;
    empty_ = false;
  }
  
  template<typename TimeType>
  TimeParamBase<TimeType>&
  TimeParamBase<TimeType>::operator++ ()
  {
    Time::operator++();
    empty_ = false;
    return *this;
  }

  template<typename TimeType>
  TimeParamBase<TimeType>&
  TimeParamBase<TimeType>::operator++ (int)
  {
    Time::operator++();
    empty_ = false;
    return *this;
  }

  template<typename TimeType>
  TimeParamBase<TimeType>&
  TimeParamBase<TimeType>::operator-- ()
  {
    Time::operator--();
    empty_ = false;
    return *this;
  }

  template<typename TimeType>
  TimeParamBase<TimeType>&
  TimeParamBase<TimeType>::operator-- (int)
  {
    Time::operator--();
    empty_ = false;
    return *this;
  }

  // RequestParam

  template <class Request, class Base>
  template <class T>
  RequestParam<Request, Base>::RequestParam(
    Request* request,
    const char* name,
    T defs,
    bool set_defs) :
    Base(request, name)
  {
    if (set_defs)
    {
      Base::set_param_val(defs);
    }
  }

  template <class Request, class Base>
  template<typename... Args>
  RequestParam<Request, Base>::RequestParam(
    Args&&... args):
    Base(std::forward<Args>(args)...)
  { }

  template <class Request, class Base>
  RequestParam<Request, Base>::~RequestParam() noexcept
  { }

  template <class Request, class Base>
  template <class T>
  RequestParam<Request, Base>&
  RequestParam<Request, Base>::operator= (
    const T& val)
  {
    Base::set_param_val(val);
    return *this;
  }

  template <class Request, class Base>
  template <class T>
  Request&
  RequestParam<Request, Base>::operator() (
    const T& val)
  {
    Base::set_param_val(val);
    return static_cast<Request&>(*Base::request_);
  }

  // HeaderParam

  template <class Request, class Base>
  template <class T>
  HeaderParam<Request, Base>::HeaderParam(
    Request* request,
    const char* name,
    const T& defs,
    bool set_defs)
    : Base(request, name)
  {
    if (set_defs)
    {
      Base::set_param_val(defs);
    }
  }

  template <class Request, class Base>
  HeaderParam<Request, Base>::HeaderParam(
    Request* request,
    const char* name,
    const String::SubString& defs,
    bool set_defs)
    : Base(request, name)
  {
    if (set_defs)
    {
      Base::set_param_val(defs);
    }
  }

  template <class Request, class Base>
  HeaderParam<Request, Base>::HeaderParam(
    Request* request,
    const HeaderParam& other)
    : Base(request, other)
  {
    set_param_val(other.raw_str());
  }

  template <class Request, class Base>
  HeaderParam<Request, Base>::HeaderParam(
    Request* request,
    const char* name)
    : Base(request, name)
  { }
 
  template <class Request, class Base>
  HeaderParam<Request, Base>::~HeaderParam() noexcept
  { }

  template <class Request, class Base>
  void
  HeaderParam<Request, Base>::set_param_val(
    const String::SubString& val)
  {
    Base::set_param_val(val);
    if (!val.empty())
    {
      static_cast<Request*>(Base::request_)->
        headers().
          remove_if(
            std::bind2nd(
              EqualHeaderName(), Base::name_));

      static_cast<Request*>(Base::request_)->
        headers().
          push_back(
            HTTP::Header(
              Base::name_, Base::param_value_));
    }
  }

  template <class Request, class Base>
  bool
  HeaderParam<Request, Base>::print(
    std::ostream&, const char*, const char*) const
  {
    return false;
  }

  template <class Request, class Base>
  void
  HeaderParam<Request, Base>::clear()
  {
    Base::clear();

    static_cast<Request*>(Base::request_)->
      headers().
        remove_if(
          std::bind2nd(
            EqualHeaderName(), Base::name_));
  }

  template <class Request, class Base>
  template <class T>
  HeaderParam<Request, Base>&
  HeaderParam<Request, Base>::operator= (
    const T& val)
  {
    Base::set_param_val(val);
    return *this;
  }

  template <class Request, class Base>
  template <class T>
  Request&
  HeaderParam<Request, Base>::operator() (
    const T& val)
  {
    Base::set_param_val(val);
    return static_cast<Request&>(*Base::request_);
  }

  template <class Request, class Base>
  HeaderParam<Request, Base>&
  HeaderParam<Request, Base>::operator= (
    const String::SubString& val)
  {
    Base::set_param_val(val);
    return *this;
  }

  template <class Request, class Base>
  Request&
  HeaderParam<Request, Base>::operator() (
    const String::SubString& val)
  {
    Base::set_param_val(val);
    return static_cast<Request&>(*Base::request_);
  }


  // ParamsGenerator
  
  template <class Param>
  template<typename... Args>
  Param* 
  ParamsGenerator<Param>::operator() (
    BaseParamsContainer*,
    Args&&... args)
  {
    return new Param(std::forward<Args>(args)...);
  }

  // ComplexParam
  
  template<typename Request, typename TKey, typename TValue, typename Generator>
  template<typename... Args>
  ComplexParam<Request, TKey, TValue, Generator>::ComplexParam(
    Args&&... args) :
    Base(std::forward<Args>(args)...)
  { }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  ComplexParam<Request, TKey, TValue, Generator>::ComplexParam(
    Request* request,
    const ComplexParam& other)
    : Base(request, other)
  {
    for (auto it = other.parameters_.begin();
         it != other.parameters_.end(); ++it)
    {
      parameters_[it->first] =
        Generator()(
          request, this, *it->second);
    }
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  ComplexParam<Request, TKey, TValue, Generator>::~ComplexParam() noexcept
  { }
  
  template<typename Request, typename TKey, typename TValue, typename Generator>
  void
  ComplexParam<Request, TKey, TValue, Generator>::clear_all()
  {
    parameters_.clear();
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  bool
  ComplexParam<Request, TKey, TValue, Generator>::clear(
    const TKey& key)
  {
    auto pos = parameters_.find(key);
    if (pos != parameters_.end())
    {
      parameters_.erase(pos);
      return true;
    }
    return false;
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  void
  ComplexParam<Request, TKey, TValue, Generator>::clear()
  {
    Base::clear();
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  bool
  ComplexParam<Request, TKey, TValue, Generator>::empty_all () const
  {
    for (auto it = parameters_.begin();  it != parameters_.end(); ++it)
    {
      if (!it->second->empty()) return false;
    }
    return true;
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  bool
  ComplexParam<Request, TKey, TValue, Generator>::empty (
    const TKey& key) const
  {
    auto pos = parameters_.find(key);
    if (pos != parameters_.end())
    {
      return pos->second->empty();
    }
    else
    {
      return true;
    }
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  bool
  ComplexParam<Request, TKey, TValue, Generator>::empty () const
  {
    return Base::empty() && empty_all();
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  TValue&
  ComplexParam<Request, TKey, TValue, Generator>::operator[] (
    TKey key)
  {
    auto pos = parameters_.find(key);
    if (pos == parameters_.end())
    {
      parameters_[key] =
        Generator()(
          this->request_, this, strof(key).c_str());
    }
    return *parameters_[key];
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  template <class T>
  Request&
  ComplexParam<Request, TKey, TValue, Generator>::operator() (
    const T& val,
    TKey  key)
  {
    set_param_val(val, key);
    return static_cast<Request&>(*this->request_);
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  template <class T>
  Request&
  ComplexParam<Request, TKey, TValue, Generator>::operator() (
    const T& val)
  {
    Base::set_param_val(val);
    return static_cast<Request&>(*this->request_);
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  template <class T>
  ComplexParam<Request, TKey, TValue, Generator>&
  ComplexParam<Request, TKey, TValue, Generator>::operator= (const T& val)
  {
    Base::set_param_val(val);
    return *this;
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  template <class T>
  void
  ComplexParam<Request, TKey, TValue, Generator>::set_param_val(
    const T& val,
    TKey key)
  {
    std::stringstream ostr;
    ostr << val;
    set_param_val(ostr.str(), key);
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  void
  ComplexParam<Request, TKey, TValue, Generator>::set_param_val(
    const std::string& val,
    TKey key)
  {
    TValue* param =
      Generator()(
        this->request_, this, strof(key).c_str());
    param->set_param_val(val);
    parameters_[key] = param;
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  bool
  ComplexParam<Request, TKey, TValue, Generator>::need_encode() const
  {
    return Base::request_->need_encode();
  }

  template<typename Request, typename TKey, typename TValue, typename Generator>
  void
  ComplexParam<Request, TKey, TValue, Generator>::add_param(
    BaseParam*)
  {
    // Do nothing
  }

  // RequestMember
  
  template <typename Request, typename Param>
  RequestMember<Request, Param>::RequestMember(
    Param Request::* member) :
    member_(member)
  { }

  template <typename Request, typename Param>
  RequestMember<Request, Param>::~RequestMember() noexcept
  { }

  template <typename Request, typename Param>
  void
  RequestMember<Request, Param>::set_param_val(
    BaseRequest& request,
    const std::string& val)
  {
    (static_cast<Request&>(request).*(member_)).
      operator()(val);
  }

  template <typename Request, typename Param>
  void
  RequestMember<Request, Param>::set_param_val(
    BaseRequest& request,
    unsigned long val)
  {
    std::ostringstream s_val;
    s_val << val;
    (static_cast<Request&>(request).*(member_)).
      operator()(s_val.str());
  }

  template <typename Request, typename Param>
  void
  RequestMember<Request, Param>::clear_param(
    BaseRequest& request)
  {
    (static_cast<Request&>(request).*(member_)).
      clear();
  }

  template <typename Request, typename Param>
  RequestMemberBase*
  RequestMember<Request, Param>::clone() const
  {
    return new RequestMember<Request, Param>(member_);
  }

  // RequestParamSetter

  template <typename Request>
  RequestParamSetter<Request>::RequestParamSetter(
    std::nullptr_t member) :
    param_(member)
  { }
  
  template <typename Request>
  template <typename Param>
  RequestParamSetter<Request>::RequestParamSetter(
    Param Request::* member) :
    param_(new RequestMember<Request, Param>(member))
  { }

  template <typename Request>
  RequestParamSetter<Request>::RequestParamSetter(
    const RequestParamSetter& other) :
    param_(other.param_? other.param_->clone(): nullptr)
  { }

  template <typename Request>
  RequestParamSetter<Request>::~RequestParamSetter() noexcept
  {
    if (param_)
    {
      delete param_;
    }
  }

  template <typename Request>
  template <typename T>
  void
  RequestParamSetter<Request>::operator() (
    Request& request,
    const T& val) const
    /*throw(eh::Exception)*/
  {
    if (param_)
    {
      param_->
        set_param_val(request, val);
    }
  }

  template <typename Request>
  void
  RequestParamSetter<Request>::clear (
    Request& request)  const
  {
    if (param_)
    {
      param_->clear_param(request);
    }
  }

  template <typename Request>
  bool
  RequestParamSetter<Request>::empty () const
  {
    return param_ == nullptr;
  }
}
