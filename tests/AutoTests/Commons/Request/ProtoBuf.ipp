
namespace AutoTest
{
  namespace ProtoBuf
  {
    // Type
    template <typename ST, typename GT>
    template <typename T>
    void
    Type<ST, GT>::set_value(
      Message* message,
      const std::string& name,
      T value)
    {
      const Descriptor* descriptor = message->GetDescriptor();
      const FieldDescriptor* field = get_field(descriptor,name);
      const Reflection* reflection = message->GetReflection();
      ((reflection)->*(setter_))(message, field, value);
    }

    template <typename ST, typename GT>
    GT
    Type<ST, GT>::get_value(
      Message* message,
      const std::string& name)
    {
      const Descriptor* descriptor = message->GetDescriptor();
      const FieldDescriptor* field = get_field(descriptor,name);
      const Reflection* reflection = message->GetReflection();
      return ((reflection)->*(getter_))(*message, field);
    }

    // Enum
    template <typename T>
    void
    Enum<T>::set_value(
      Message* message,
      const std::string& name,
      T value)
    {
      const EnumValueDescriptor* value_descr =
        descriptor_->FindValueByNumber(value);
      EnumType().set_value(message, name, value_descr);
    }

    template <typename T>
    T
    Enum<T>::get_value(
      Message* message,
      const std::string& name)
    {
      const EnumValueDescriptor* value = 
        EnumType().get_value(message, name);
      return static_cast<T>(value->number());
    }
    
  }
  
  // class BidParam

  template<typename Request, typename Setter>
  template <typename T>
  BidParam<Request, Setter>::BidParam(
    Request* request,
    Message* message,
    const char* name,
    T def,
    bool set_defs) :
    BaseParam(request, name),
    message_(message)
  {
    if (set_defs)
    {
      set_param_val(def);
    }
  }
  
  template<typename Request, typename Setter>
  BidParam<Request, Setter>::BidParam(
    Request* request,
    Message* message,
    const char* name) :
    BaseParam(request, name),
    message_(message)
  { }
      
  template<typename Request, typename Setter>
  BidParam<Request, Setter>::BidParam(
    Request* request,
    Message* message,
    const BidParam& other) :
    BaseParam(request, other),
    message_(message)
  { }

  template<typename Request, typename Setter>
  BidParam<Request, Setter>::~BidParam() noexcept
  { }


  template<typename Request, typename Setter>
  void BidParam<Request, Setter>::clear()
  {
    ProtoBuf::clear(message_, name_);
  }
  
  template<typename Request, typename Setter>
  template <typename T>
  void
  BidParam<Request, Setter>::set_param_val(
    T val)
  {
    Setter().set_value(message_, name_, val);
  }

  template <typename Request, typename Setter>
  bool
  BidParam<Request, Setter>::print(
    std::ostream&, const char*, const char*) const
  {
    return false;
  }

  template <typename Request, typename Setter>
  bool
  BidParam<Request, Setter>::empty() const
  {
    return ProtoBuf::empty(message_, name_);
  }

  template <typename Request, typename Setter>
  std::string
  BidParam<Request, Setter>::str() const
  {
    throw NotSupported("BidParam::str() not supported");
  }

  template <typename Request, typename Setter>
  std::string
  BidParam<Request, Setter>::raw_str() const
  {
    throw NotSupported("BidParam::raw_str() not supported");
  }

  template <typename Request, typename Setter>
  void
  BidParam<Request, Setter>::set_param_val(
    const String::SubString&)
  {
    throw NotSupported(
      "BidParam::set_param_val(const String::SubString&) "
      "not supported");
  }

  template <typename Request, typename Setter>
  template <typename T>
  BidParam<Request, Setter>&
  BidParam<Request, Setter>::operator=(
    const T& val)
  {
    set_param_val(val);
    return *this;
  }

  template <typename Request, typename Setter>
  template <typename T>
  Request&
  BidParam<Request, Setter>::operator() (
    const T& val)
  {
    set_param_val(val);
    return static_cast<Request&>(*BaseParam::request_);
  }

  template <typename Request, typename Setter>
  typename Setter::NestedType 
  BidParam<Request, Setter>::operator*()
  {
    return get();
  }

  template <typename Request, typename Setter>
  typename Setter::NestedType
  BidParam<Request, Setter>::get()
  {
    return Setter().get_value(message_, name_);
  }
  template <typename Request, typename Setter>
  const std::string&
  BidParam<Request, Setter>::name() const
  {
    return BaseParam::name_;
  }

  //  RequestMember for BidParam

  template <typename Request, typename Setter>
  template<typename T, typename Arg>
  void
  RequestMember< Request, BidParam<Request,Setter> >::
  MemberSetter::set_int(
    T& param,
    Arg arg,
    typename std::enable_if<
      std::is_integral<typename T::Type>::value>::type*)
  {
    param.operator()(
      static_cast<typename T::Type>(arg));
  }

  template <typename Request, typename Setter>
  template<typename T, typename Arg>
  void
  RequestMember< Request, BidParam<Request,Setter> >::
  MemberSetter::set_int(
    T& param,
    Arg arg,
    typename std::enable_if<
      !std::is_integral<typename T::Type>::value>::type*)
  {
    std::stringstream s;
    s << arg;
    param.operator()(s.str());
  }
  
  template <typename Request, typename Setter>
  template<typename T>
  void
  RequestMember< Request, BidParam<Request,Setter> >::
  MemberSetter::set_string(
    T& param,
    const std::string& arg,
    typename std::enable_if<
      std::is_integral<typename T::Type>::value>::type*)
  {
    std::istringstream is(arg);
    typename T::Type v;
    is >> v;
    if (is.fail())
    {
      Stream::Error error;
      error << "Invalid protobuf parameter '" <<
        param.name() << "' value: " << arg;
      throw InvalidParameter(error);
    }
    param.operator()(v);
  }
  
  template <typename Request, typename Setter>
  template<typename T>
  void
  RequestMember< Request, BidParam<Request,Setter> >::
  MemberSetter::set_string(
    T& param,
    const std::string& arg,
    typename std::enable_if<
      !std::is_integral<typename T::Type>::value>::type*)
  {
    param.operator()(arg);
  }
  
  template <typename Request, typename Setter>
  RequestMember< Request, BidParam<Request,Setter> >::
  RequestMember(Param Request::* member) :
    member_(member)
  { }

  template <typename Request, typename Setter>
  RequestMember< Request, BidParam<Request, Setter> >::
  ~RequestMember() noexcept
  { }

  template <typename Request, typename Setter>
  void
  RequestMember< Request, BidParam<Request,Setter> >::
  set_param_val(
    BaseRequest& request,
    const std::string& val)
  {
    MemberSetter().
      set_string(
        static_cast<Request&>(request).*(member_),
        val);
  }

  template <typename Request, typename Setter>
  void
  RequestMember< Request, BidParam<Request,Setter> >::
  set_param_val(
    BaseRequest& request,
    unsigned long val)
  {
    MemberSetter().
      set_int(
        static_cast<Request&>(request).*(member_),
        val);
  }

  template <typename Request, typename Setter>
  void
  RequestMember< Request, BidParam<Request,Setter> >::
  clear_param(
    BaseRequest& request)
  {
    (static_cast<Request&>(request).*(member_)).
      clear();
  }

  template <typename Request, typename Setter>
  RequestMemberBase*
  RequestMember< Request, BidParam<Request,Setter> >::
  clone() const
  {
    return new RequestMember<Request, Param>(member_);
  }
    
}
