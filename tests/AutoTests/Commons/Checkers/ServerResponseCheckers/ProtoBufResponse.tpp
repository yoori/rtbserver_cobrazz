
namespace AutoTest
{
  namespace ProtoBuf
  {

    // FieldChecker
    template <typename ProtoBufChecker, typename T, typename Getter>      
    FieldChecker<ProtoBufChecker, T, Getter>::FieldChecker(
      ProtoBufChecker& checker,
      const T& exp,
      Getter getter) :
      checker_(checker),
      getter_(getter)
    {
      exp_ = exp;
    }
    
    template <typename ProtoBufChecker, typename T, typename Getter>      
    FieldChecker<ProtoBufChecker, T, Getter>::FieldChecker(
      const FieldChecker& other) :
      Checker(),
      checker_(other.checker_),
      getter_(other.getter_)
    {
      exp_ = other.exp_;
    }

    template <typename ProtoBufChecker, typename T, typename Getter>      
    FieldChecker<ProtoBufChecker, T, Getter>::~FieldChecker() noexcept
    { }
    
    template <typename ProtoBufChecker, typename T, typename Getter>      
    bool
    FieldChecker<ProtoBufChecker, T, Getter>::check(
      bool throw_error)
      /*throw(eh::Exception)*/
    {
      T got;
      got = ((checker_.ad()).*getter_)();

      if (!equal(exp_, got))
      {
        if (throw_error)
        {
          Stream::Error err;
          err << strof(exp_) << " != " << strof(got);
          throw AutoTest::CheckFailed(err);
        }
        return false;
      }
      return true;
    }

    // FieldExistChecker

    template <typename ProtoBufChecker, typename Getter>      
    FieldExistChecker<ProtoBufChecker, Getter>::FieldExistChecker(
      ProtoBufChecker& checker,
      bool exist,
      Getter getter) :
      checker_(checker),
      exist_(exist),
      getter_(getter)
    { }

    template <typename ProtoBufChecker, typename Getter>      
    FieldExistChecker<ProtoBufChecker, Getter>::~FieldExistChecker() noexcept
    { }

    template <typename ProtoBufChecker, typename Getter>      
    bool
    FieldExistChecker<ProtoBufChecker, Getter>::check(
      bool throw_error)
      /*throw(eh::Exception)*/
    {
      bool got_exist =
        static_cast<bool>(
          ((checker_.ad()).*getter_)());
      if (exist_ ^ got_exist)
      {
        if (throw_error)
        {
          Stream::Error err;
          err << (exist_? "not exist": "exist");
          throw AutoTest::CheckFailed(err);
        }
        return false;
      }

      return true;
    }

    // Expected

    template<typename Message>
    template <typename ProtoBufChecker, typename T, typename Getter>
    const ExpValue<T>&
    ExpectedUtils< Message>::add_checker(
      ProtoBufChecker& checker,
      const std::string& name,
      const ExpValue<T>& val,
      Getter getter)
    {
      if (val.is_set())
      {
        typedef FieldChecker<ProtoBufChecker, T, Getter> CheckerType;
        checker.checkers_.push_back(
          typename ProtoBufChecker::CheckerPair(
            name,
            new AutoTest::Internals::CheckerHolderImpl<CheckerType>(
              CheckerType(checker, *val, getter))));
      }
      return val;
    }

    template<typename Message>
    template <typename ProtoBufChecker, typename SeqIn, typename SeqOut>
    const ExpValue<SeqIn>&
    ExpectedUtils<Message>::add_seq_checker(
      ProtoBufChecker& checker,
      const std::string& name,
      const ExpValue<SeqIn>& val,
      const SeqOut& (Message::*getter)(void) const)
    {
      if (val.is_set())
      {
        
        typedef const SeqOut& (Message::*Getter)(void) const;
        typedef FieldChecker<ProtoBufChecker, SeqOut, Getter> CheckerType;
        SeqOut seq(val->begin(), val->end());
        checker.checkers_.push_back(
          typename ProtoBufChecker::CheckerPair(
            name,
            new AutoTest::Internals::CheckerHolderImpl<CheckerType>(
              CheckerType(checker, seq, getter))));
      }

      return val;
    }

    template<typename Message>
    template <typename ProtoBufChecker, typename T>
    const ExpValue<bool>&
    ExpectedUtils<Message>::add_exist_checker(
        ProtoBufChecker& checker,
        const std::string& name,
        const ExpValue<bool>& val,
        T (Message::*getter)(void) const)
    {
      if (val.is_set())
      {
        typedef T (Message::*Getter)(void) const;
        typedef FieldExistChecker<ProtoBufChecker, Getter> CheckerType;
        
        checker.checkers_.push_back(
          typename ProtoBufChecker::CheckerPair(
            name,
            new AutoTest::Internals::CheckerHolderImpl<CheckerType>(
              CheckerType(checker, *val, getter))));
      }
      return val;
    }

    // AdChecker
    
    template<typename Tag>
    AdChecker<Tag>::AdChecker(
      const AdClient& client,
      const Expected& expected,
      size_t creative_num) :
      expected_(static_cast<Tag&>(*this), expected),
      creative_num_(creative_num)
    {
      response_.ParseFromString(client.req_response_data());
    }
    
    template<typename Tag>
    AdChecker<Tag>::AdChecker(
      const AdChecker& other) :
      Checker(),
      response_(other.response_),
      expected_(static_cast<Tag&>(*this), other.expected_),
      creative_num_(other.creative_num_)
    {  }
    
    template<typename Tag>
    AdChecker<Tag>::~AdChecker() noexcept
    { }
    
    template<typename Tag>
    bool
    AdChecker<Tag>::check(
      bool throw_error)
      /*throw(CheckFailed, eh::Exception)*/
    {
      if (
        (creative_num_ + 1 > ad_size() && !checkers_.empty()) ||
        (ad_size() && checkers_.empty() && !creative_num_))
      {
        if (throw_error)
        {
          Stream::Error error;
          error << "Got unexpected bids size: got " << ad_size()
                << " bids, but expected ";
          if (checkers_.empty())
          { error << "0."; }
          else
          { error << "at least " << (creative_num_ + 1) << "."; }
          throw CheckFailed(error);
        }
        return false;
      }

      std::list<std::pair<std::string, CheckFailed>> errors;
      bool result = true;
      for(auto it = checkers_.begin();it != checkers_.end(); ++it )
      {
        try
        {
          result &= it->second->check(throw_error);
        }
        catch(const CheckFailed& ex)
        {
          errors.push_back(std::make_pair(it->first, ex));
          result = false;
        }
      }

      
      if(throw_error && !result)
      {
        Stream::Error ostr;
        ostr << "Ad response creative#" << creative_num_ << ":";
        for(auto ex_it = errors.begin(); ex_it != errors.end(); ++ex_it)
        {
          ostr << std::endl << "  " << ex_it->first << ": " << ex_it->second.what();
        }
        
        throw CheckFailed(ostr);
      }

      return result;
    }
  }
    
  // TypeTraits

  // RepeatedPtrField specialization
  
  template<typename T>
  bool
  TypeTraits< google::protobuf::RepeatedPtrField<T> >::equal(
    const SeqType& exp,
    const SeqType& got)
  {
    return equal_seq(exp, got);
  }
  
  template<typename T>
  std::string
  TypeTraits< google::protobuf::RepeatedPtrField<T> >::to_string(
    const SeqType& val)
  {
    return seq_to_str(val);
  }
  
  // RepeatedField specialization
  
  template<typename T>
  bool
  TypeTraits< google::protobuf::RepeatedField<T> >::equal(
    const SeqType& exp,
    const SeqType& got)
  {
    return equal_seq(exp, got);
  }
  
  template<typename T>
  std::string
  TypeTraits< google::protobuf::RepeatedField<T> >::to_string(
    const SeqType& val)
  {
    return seq_to_str(val);
  }
  
}


