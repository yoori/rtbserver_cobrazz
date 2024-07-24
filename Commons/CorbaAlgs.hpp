#ifndef _CORBAALGS_HPP_
#define _CORBAALGS_HPP_

#include <cassert>
#include <sstream>
#include <optional>

#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>
#include <Commons/CorbaTypes.hpp>
#include <CORBA/CORBACommons/CorbaAdapters.hpp>

#include "Containers.hpp"
#include "UserInfoManip.hpp"

namespace CorbaAlgs
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  template <typename InType>
  const InType&
  fill_type_adapter(const InType& val)
  {
    return val;
  }

  inline
  const char*
  fill_type_adapter(const std::string& val)
  {
    return val.c_str();
  }

  inline
  const char*
  fill_type_adapter(const Generics::StringHashAdapter& val)
  {
    return val.text().c_str();
  }

  template <typename InType>
  const InType&
  convert_type_adapter(const InType& val)
  {
    return val;
  }

  inline
  const char*
  convert_type_adapter(const TAO::String_Manager& val)
  {
    return val.in();
  }

  inline
  const char*
  convert_type_adapter(const TAO::unbounded_basic_string_sequence<
    CORBA::Char>::const_element_type& val)
  {
    return val.in();
  }

  inline
  Generics::Time
  unpack_time(const CORBACommons::TimestampInfo& ts)
  {
    Generics::Time ret;
    assert(Generics::Time::TIME_PACK_LEN == ts.length());
    ret.unpack(ts.get_buffer());
    return ret;
  }

  inline
  CORBACommons::TimestampInfo
  pack_time(const Generics::Time& ts)
  {
    CORBACommons::TimestampInfo ret;
    assert(Generics::Time::TIME_PACK_LEN == 8);
    ret.length(Generics::Time::TIME_PACK_LEN);
    ts.pack(ret.get_buffer());
    return ret;
  }

  static_assert(Generics::Time::TIME_PACK_LEN == 8,
    "Unexpected TIME_PACK_LEN value");

  inline
  Generics::Time
  unpack_time(const CORBACommons::FixedTimestampInfo& ts)
  {
    Generics::Time ret;
    ret.unpack(ts);
    return ret;
  }

  inline void
  pack_time(CORBACommons::FixedTimestampInfo& cts, const Generics::Time& ts)
  {
    ts.pack(cts);
  }

  inline
  AdServer::Commons::UserId
  unpack_user_id(const CORBACommons::UserIdInfo& uid)
    /*throw(eh::Exception)*/
  {
    if(uid.length())
    {
      return AdServer::Commons::UserId(
        uid.get_buffer(), uid.get_buffer() + uid.length());
    }
    else
    {
      return AdServer::Commons::UserId();
    }
  }

  inline
  CORBACommons::UserIdInfo
  pack_user_id(const AdServer::Commons::UserId& user_id)
  {
    CORBACommons::UserIdInfo ret;
    ret.length(user_id.size());
    std::copy(user_id.begin(), user_id.end(), ret.get_buffer());
    return ret;
  }

  inline
  AdServer::Commons::RequestId
  unpack_request_id(const CORBACommons::RequestIdInfo& rid)
    /*throw(eh::Exception)*/
  {
    return unpack_user_id(rid);
  }

  inline
  CORBACommons::RequestIdInfo
  pack_request_id(const AdServer::Commons::RequestId& request_id)
  {
    return pack_user_id(request_id);
  }

  template<typename DecimalType>
  DecimalType
  unpack_decimal(const CORBACommons::DecimalInfo& dec)
  {
    DecimalType ret;
    assert(DecimalType::PACK_SIZE == dec.length());
    ret.unpack(dec.get_buffer());
    return ret;
  }

  template<typename DecimalType>
  std::optional<DecimalType>
  unpack_optional_decimal(const CORBACommons::DecimalInfo& dec)
  {
    if(dec.length())
    {
      DecimalType ret;
      assert(DecimalType::PACK_SIZE == dec.length());
      ret.unpack(dec.get_buffer());
      return std::optional<DecimalType>(ret);
    }
    else
    {
      return std::optional<DecimalType>();
    }
  }

  // non explicit OctSeq(CORBA::ULong) c-tor workaround
  template<typename DecimalType>
  DecimalType
  unpack_decimal(CORBA::ULong);

  template<typename DecimalType>
  CORBACommons::DecimalInfo
  pack_decimal(const DecimalType& dec)
  {
    CORBACommons::DecimalInfo ret;
    ret.length(DecimalType::PACK_SIZE);
    dec.pack(ret.get_buffer());
    return ret;
  }

  template<typename DecimalType>
  CORBACommons::OptionalDecimalInfo
  pack_optional_decimal(
    const std::optional<DecimalType>& dec)
  {
    CORBACommons::OptionalDecimalInfo ret;
    if(dec)
    {
      ret.length(DecimalType::PACK_SIZE);
      dec->pack(ret.get_buffer());
    }
    return ret;
  }

  /** utils */
  template<
    typename TargetType,
    typename SourceIteratorType>
  void fill_sequence(
    const SourceIteratorType& begin,
    const SourceIteratorType& end,
    TargetType& target_seq,
    bool add = false) /*throw(CORBA::SystemException)*/
  {
    std::size_t dist = std::distance(begin, end);

    CORBA::ULong i = add ? target_seq.length() : 0;
    target_seq.length(i + dist);

    for (SourceIteratorType it = begin; it != end; ++it, ++i)
    {
      target_seq[i] = fill_type_adapter(*it);
    }
  }

  template<
    typename TargetType,
    typename SourceIteratorType>
  void fill_unique_sequence(
    const SourceIteratorType& begin,
    const SourceIteratorType& end,
    TargetType& target_seq) /*throw(CORBA::SystemException)*/
  {
    std::size_t dist = std::distance(begin, end);
    target_seq.length(dist);
    if(dist>0)
    {
      SourceIteratorType it = begin;
      target_seq[0] = *it++;
      CORBA::ULong i = 1;
      while(it!=end)
      {
        if(!(target_seq[i - 1] == *it))
        {
          target_seq[i++] = *it;
        }
        it++;
      }
      target_seq.length(i);
    }
  }

  template<
    typename TargetContainerType,
    typename SourceSeqType>
  void convert_sequence(
    const SourceSeqType& source_seq,
    TargetContainerType& target_cont) /*throw(CORBA::SystemException)*/
  {
    std::insert_iterator<TargetContainerType> ins_it(
      target_cont, target_cont.end());

    for (CORBA::ULong i = 0; i < source_seq.length(); ++i)
    {
      *ins_it = convert_type_adapter(source_seq[i]);
      ++ins_it;
    }
  }

  template<
    typename InsertIteratorType,
    typename SourceSeqType>
  InsertIteratorType convert_sequence_it(
    const SourceSeqType& source_seq,
    InsertIteratorType ins_it) /*throw(CORBA::SystemException)*/
  {
    for(CORBA::ULong i = 0; i < source_seq.length(); ++i)
    {
      *ins_it = convert_type_adapter(source_seq[i]);
      ++ins_it;
    }
    return ins_it;
  }

  template<
    typename TargetType,
    typename SourceSeqType>
  void copy_sequence(
    const SourceSeqType& source_seq,
    TargetType& target_seq,
    bool add = false) /*throw(CORBA::SystemException)*/
  {
    if(add)
    {
      CORBA::ULong prev_len = target_seq.length();
      target_seq.length(prev_len + source_seq.length());
      for(CORBA::ULong i = 0; i < source_seq.length(); ++i)
      {
        target_seq[prev_len + i] = source_seq[i];
      }
    }
    else
    {
      target_seq.length(source_seq.length());
      for(CORBA::ULong i = 0; i < source_seq.length(); ++i)
      {
        target_seq[i] = source_seq[i];
      }
    }
  }

  template<typename OStream, typename SourceSeqType>
  void print_sequence(
    OStream& out,
    const SourceSeqType& source_seq,
    const char* delim = ",")
    /*throw(CORBA::SystemException)*/
  {
    for(CORBA::ULong i = 0; i < source_seq.length(); ++i)
    {
      if(i != 0)
      {
        out << delim;
      }

      out << source_seq[i];
    }
  }

  template<typename SourceSeqType, typename FieldType>
  void print_sequence_sequence(
    std::ostream& out,
    const SourceSeqType& source_seq,
    FieldType SourceSeqType::value_type::* field,
    const char* delim = ",")
    /*throw(CORBA::SystemException)*/
  {
    std::streampos p = out.tellp();
    for (CORBA::ULong i = 0; i < source_seq.length(); ++i)
    {
      std::ostringstream ostr;
      CorbaAlgs::print_sequence(ostr, source_seq[i].*field);

      if (ostr.tellp() > 0)
      {
        if (p != out.tellp())
        {
          out << delim;
        }
        out << ostr.str();
        p = out.tellp();
      }
    }
  }

  template<typename SourceSeqType, typename FieldType>
  void print_sequence_field(
    std::ostream& out,
    const SourceSeqType& source_seq,
    FieldType SourceSeqType::value_type::* field,
    const char* delim = ",")
    /*throw(CORBA::SystemException)*/
  {
    for(CORBA::ULong i = 0; i < source_seq.length(); ++i)
    {
      if(i != 0)
      {
        out << delim;
      }

      out << source_seq[i].*field;
    }
  }

  template<typename SourceSeqType, typename FieldType>
  void print_sequence_fields(
    std::ostream& out,
    const SourceSeqType& source_seq,
    FieldType SourceSeqType::value_type::* field1,
    FieldType SourceSeqType::value_type::* field2,
    const char* delim = ",",
    const char* field_delim = " : ")
    /*throw(CORBA::SystemException)*/
  {
    for(CORBA::ULong i = 0; i < source_seq.length(); ++i)
    {
      if(i != 0)
      {
        out << delim;
      }

      out << source_seq[i].*field1 << field_delim << source_seq[i].*field2;
    }
  }

  inline
  void
  convert_mem_buf(
    CORBACommons::OctSeq& oct_seq,
    const Generics::MemBuf& mem_buf)
    /*throw(Exception)*/
  {
    try
    {
      oct_seq.length(mem_buf.size());
      ::memcpy(oct_seq.get_buffer(), mem_buf.data(), mem_buf.size());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << "convert_mem_buf(): Caught CORBA::SystemException: " << ex;
      throw Exception(ostr);
    }
  }

  inline
  void
  conver_string(
    CORBACommons::OctSeq& oct_seq,
    const std::string& data)
  {
    try
    {
      oct_seq.length(data.size());
      ::memcpy(oct_seq.get_buffer(), data.data(), data.size());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << "conver_string(): Caught CORBA::SystemException: " << ex;
      throw Exception(ostr);
    }
  }

  inline
  Generics::SmartMemBuf_var
  unpack_mem_buf(const CORBACommons::OctSeq& oct_seq) /*throw(eh::Exception)*/
  {
    Generics::SmartMemBuf_var res(new Generics::SmartMemBuf(oct_seq.length()));
    res->membuf().assign(oct_seq.get_buffer(), oct_seq.length());
    return res;
  }

  template<typename ULongSeqType, typename DecimalType>
  void
  pack_decimal_into_seq(
    ULongSeqType& seq,
    const DecimalType& value)
  {
    const unsigned long EL_NUMBER = DecimalType::PACK_SIZE / 4 +
      (DecimalType::PACK_SIZE % 4 ? 1 : 0);
    uint32_t buf[EL_NUMBER];
    ::memset(buf, 0, EL_NUMBER * 4);
    value.pack(buf);

    CORBA::ULong pos = seq.length();
    seq.length(pos + EL_NUMBER + 1);
    seq[pos] = 0;
    for(CORBA::ULong i = 0; i < EL_NUMBER; ++i)
    {
      seq[pos + i + 1] = buf[i];
    }
  }

  template<typename ULongSeqType, typename DecimalType>
  void
  unpack_decimal_from_seq(
    DecimalType& value,
    const ULongSeqType& seq,
    CORBA::ULong pos)
  {
    const unsigned long EL_NUMBER = DecimalType::PACK_SIZE / 4 +
      (DecimalType::PACK_SIZE % 4 ? 1 : 0);
    uint32_t buf[EL_NUMBER];

    assert(seq.length() - pos == EL_NUMBER);

    for(CORBA::ULong i = 0; i < EL_NUMBER; ++i)
    {
      buf[i] = seq[pos + i];
    }

    value.unpack(buf);
  }
}

#endif /*_CORBAALGS_HPP_*/
