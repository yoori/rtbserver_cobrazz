#ifndef _GRPCALGS_HPP_
#define _GRPCALGS_HPP_

// STD
#include <cassert>
#include <type_traits>
#include <optional>

// PROTOBUF
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/repeated_ptr_field.h>

// THIS
#include <Commons/Containers.hpp>
#include <Commons/UserInfoManip.hpp>

// UNIX_COMMONS
#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

namespace GrpcAlgs
{

DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

template<class T>
struct AlwaysFalse : std::false_type
{
};

template<class T>
constexpr auto AlwaysFalseV = AlwaysFalse<T>::value;

inline std::string pack_time(const Generics::Time& time)
{
  std::string result(Generics::Time::TIME_PACK_LEN, '0');
  time.pack(result.data());
  return result;
}

inline Generics::Time unpack_time(const std::string& data)
{
  assert(Generics::Time::TIME_PACK_LEN == data.length());

  Generics::Time time;
  time.unpack(data.c_str());

  return time;
}

inline std::string pack_user_id(const AdServer::Commons::UserId& user_id)
{
  std::string result;
  result.resize(user_id.size());
  std::copy(user_id.begin(), user_id.end(), result.data());
  return result;
}


inline AdServer::Commons::UserId unpack_user_id(const std::string& user_id)
{
  if(user_id.empty())
  {
    return AdServer::Commons::UserId();
  }
  else
  {
    return AdServer::Commons::UserId(
      user_id.data(),
      user_id.data() + user_id.size());
  }
}

inline std::string pack_request_id(const AdServer::Commons::RequestId& request_id)
{
  return pack_user_id(request_id);
}

inline AdServer::Commons::RequestId unpack_request_id(const std::string& request_id)
{
  return unpack_user_id(request_id);
}

template<class OStream, class RepeatedType, class MemPointerFunction, class ...MemPointersFunction>
inline void print_repeated_fields(
  OStream& out,
  const char* delim,
  [[maybe_unused]] const char* field_delim,
  const RepeatedType& repeated_value,
  MemPointerFunction pointer,
  MemPointersFunction ...pointers)
{
  static_assert(
    (std::is_member_function_pointer_v<MemPointerFunction>),
    "Pointer must be pointer to member function");

  if constexpr (sizeof...(pointers) >= 1)
  {
    static_assert(
      (std::is_member_function_pointer_v<MemPointersFunction> && ...),
      "Pointer must be pointer to member function");
  }

  const int size = repeated_value.size();
  for (int i = 0; i < size; ++i)
  {
    if (i != 0)
    {
      out << delim;
    }

    out << (repeated_value[i].*pointer)();
    ((out << field_delim << (repeated_value[i].*pointers)()), ...);
  }
}

template<class OStream, class RepeatedType>
inline void print_repeated(
  OStream& out,
  const char* delim,
  const RepeatedType& repeated_value)
{
  const int size = repeated_value.size();
  for (int i = 0; i < size; ++i)
  {
    if (i != 0)
    {
      out << delim;
    }

    out << repeated_value[i];
  }
}

template<class DecimalType>
inline std::string pack_optional_decimal(
  const std::optional<DecimalType>& data)
{
  std::string result;
  if (data)
  {
    result.resize(DecimalType::PACK_SIZE);
    data->pack(result.data());
  }
  return result;
}

template<class DecimalType>
inline std::string pack_decimal(const DecimalType& data)
{
  std::string result;
  result.resize(DecimalType::PACK_SIZE);
  data.pack(result.data());
  return result;
}

template<class DecimalType>
inline DecimalType unpack_decimal(const std::string& data)
{
  DecimalType result;
  assert(DecimalType::PACK_SIZE == data.size());
  result.unpack(data.data());
  return result;
}

template<class DecimalType>
inline DecimalType unpack_optional_decimal(const std::string& data)
{
  if (data.empty())
  {
    return std::optional<DecimalType>();
  }
  else
  {
    DecimalType result;
    assert(DecimalType::PACK_SIZE == data.size());
    result.unpack(data.data());
    return result;
  }
}

template<class It, class Type>
void fill_repeated_field(
  const It& begin,
  const It& end,
  google::protobuf::RepeatedField<Type>& result)
{
  result.Add(begin, end);
}

template<class T, class DecimalType>
void pack_decimal_into_repeated_field(
  google::protobuf::RepeatedField<T>& seq,
  const DecimalType& value)
{
  const int EL_NUMBER = DecimalType::PACK_SIZE / 4 +
    (DecimalType::PACK_SIZE % 4 ? 1 : 0);
  uint32_t buf[EL_NUMBER];
  ::memset(buf, 0, EL_NUMBER * 4);
  value.pack(buf);

  auto pos = seq.size();
  seq.Reserve(pos + EL_NUMBER + 1);
  seq.Add(0);
  seq.Add(buf, buf + EL_NUMBER);
}

template<class E>
concept ConceptError1 = requires(E e)
{
  std::decay_t<decltype(e.type())>::Error_Type_ChunkNotFound;
  std::decay_t<decltype(e.type())>::Error_Type_NotReady;
  std::decay_t<decltype(e.type())>::Error_Type_Implementation;
};

template<ConceptError1 Error>
void print_grpc_error(
  const Error& error,
  Logging::Logger* logger,
  const char* aspect) noexcept
{
  using ErrorType = typename Error::Type;

  try
  {
    switch (error.type())
    {
      case ErrorType::Error_Type_ChunkNotFound:
      {
        Stream::Error stream;
        stream << FNS
               << "Chunk not found.";
        logger->error(stream.str(), aspect);
        break;
      }
      case ErrorType::Error_Type_NotReady:
      {
        Stream::Error stream;
        stream << FNS
               << "Not ready.";
        logger->error(stream.str(), aspect);
        break;
      }
      case ErrorType::Error_Type_Implementation:
      {
        Stream::Error stream;
        stream << FNS
               << error.description();
        logger->error(stream.str(), aspect);
        break;
      }
      default:
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown error type";
        logger->error(stream.str(),aspect);
      }
    }
  }
  catch (...)
  {
  }
}

template<class E>
concept ConceptError2 = requires(E e)
{
  std::decay_t<decltype(e.type())>::Error_Type_NotReady;
  std::decay_t<decltype(e.type())>::Error_Type_IncorrectArgument;
  std::decay_t<decltype(e.type())>::Error_Type_Implementation;
};

template<ConceptError2 Error>
void print_grpc_error(
  const Error& error,
  Logging::Logger* logger,
  const char* aspect) noexcept
{
  using ErrorType = typename Error::Type;

  try
  {
    switch (error.type())
    {
      case ErrorType::Error_Type_NotReady:
      {
        Stream::Error stream;
        stream << FNS
               << "Not ready.";
        logger->error(stream.str(), aspect);
        break;
      }
      case ErrorType::Error_Type_IncorrectArgument:
      {
        Stream::Error stream;
        stream << FNS
               << "Not ready.";
        logger->error(stream.str(), aspect);
        break;
      }
      case ErrorType::Error_Type_Implementation:
      {
        Stream::Error stream;
        stream << FNS
               << error.description();
        logger->error(stream.str(), aspect);
        break;
      }
      default:
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown error type";
        logger->error(stream.str(), aspect);
      }
    }
  }
  catch (...)
  {
  }
}

template<class E>
concept ConceptError3 = requires(E e)
{
  std::decay_t<decltype(e.type())>::Error_Type_Implementation;
  std::decay_t<decltype(e.type())>::Error_Type_NotConfigured;
};

template<ConceptError3 Error>
void print_grpc_error(
  const Error& error,
  Logging::Logger* logger,
  const char* aspect) noexcept
{
  using ErrorType = typename Error::Type;

  try
  {
    switch (error.type())
    {
      case ErrorType::Error_Type_NotConfigured:
      {
        Stream::Error stream;
        stream << FNS
               << "Not configured.";
        logger->error(stream.str(), aspect);
        break;
      }
      case ErrorType::Error_Type_Implementation:
      {
        Stream::Error stream;
        stream << FNS
               << error.description();
        logger->error(stream.str(), aspect);
        break;
      }
      default:
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown error type";
        logger->error(stream.str(), aspect);
      }
    }
  }
  catch (...)
  {
  }
}

template<class ResponsePtr>
void print_grpc_error_response(
  const ResponsePtr& response,
  Logging::Logger* logger,
  const char* aspect) noexcept
{
  try
  {
    if (!response)
    {
      Stream::Error stream;
      stream << FNS
             << ": Internal grpc error";
      logger->error(stream.str(), aspect);
    }
    else
    {
      if (response->has_error())
      {
        print_grpc_error(response->error(), logger, aspect);
      }
    }
  }
  catch (...)
  {
  }
}

} // namespace GrpcAlgs

#endif /*_GRPCALGS_HPP_*/
