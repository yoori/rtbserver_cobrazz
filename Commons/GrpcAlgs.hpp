#ifndef _GRPCALGS_HPP_
#define _GRPCALGS_HPP_

// STD
#include <cassert>

// THIS
#include <Commons/UserInfoManip.hpp>

// UNIX_COMMONS
#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

namespace GrpcAlgs
{

DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

template<class T>
struct always_false : std::false_type
{
};

template<class T>
constexpr auto always_false_v = always_false<T>::value;

inline
std::string
pack_time(const Generics::Time& time)
{
  std::string result(Generics::Time::TIME_PACK_LEN, '0');
  time.pack(result.data());
  return result;
}

inline
Generics::Time
unpack_time(const std::string& data)
{
  assert(Generics::Time::TIME_PACK_LEN == data.length());

  Generics::Time time;
  time.unpack(data.c_str());

  return time;
}

inline
AdServer::Commons::UserId
unpack_user_id(const std::string& user_id)
{
  if(!user_id.empty())
  {
    return AdServer::Commons::UserId(
      user_id.data(),
      user_id.data() + user_id.size());
  }
  else
  {
    return AdServer::Commons::UserId();
  }
}

inline
AdServer::Commons::RequestId
unpack_request_id(const std::string& request_id)
{
  return unpack_user_id(request_id);
}

template<class RepeatedType, class MemPointerFunction, class ...MemPointersFunction>
inline void print_repeated_fields(
  std::ostream& out,
  const char* delim,
  const char* field_delim,
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

template<class RepeatedType>
inline void print_repeated(
  std::ostream& out,
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

template<class Error>
inline void print_grpc_error(
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
               << ": Chunk not found.";
        logger->error(
          stream.str(),
          aspect);
        break;
      }
      case ErrorType::Error_Type_NotReady:
      {
        Stream::Error stream;
        stream << FNS
               << ": Not ready.";
        logger->error(
          stream.str(),
          aspect);
        break;
      }
      case ErrorType::Error_Type_Implementation:
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << error.description();
        logger->error(
          stream.str(),
          aspect);
        break;
      }
      default:
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error type";
        logger->error(
          stream.str(),
          aspect);
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
        print_grpc_error(
          response->error(),
          logger,
          aspect);
      }
    }
  }
  catch (...)
  {
  }
}

} // namespace GrpcAlgs

#endif /*_GRPCALGS_HPP_*/