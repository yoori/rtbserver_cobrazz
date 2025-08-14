#pragma once

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <String/SubString.hpp>
#include <String/StringManip.hpp>

// RAPIDJSON
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// STD
#include <memory>
#include <sstream>
#include <utility>

namespace AdServer::Commons
{
// Write Bid Response in JSON format in stream on the fly.

namespace Internal
{

template<class T, class = void>
struct has_member_str : std::false_type
{
};

template<class T>
struct has_member_str<T, std::enable_if_t<std::is_same_v<decltype(std::declval<T&>().str()), std::string>>> : std::true_type
{
};

template<class T>
constexpr bool has_member_str_v = has_member_str<T>::value;

} // namespace Internal

class JsonObject : private Generics::Uncopyable
{
private:
  using StringBuffer = rapidjson::StringBuffer;
  using StringBufferOpt = std::optional<StringBuffer>;
  using Writer = rapidjson::Writer<StringBuffer>;
  using WriterOpt = std::optional<Writer>;

  enum ObjectType
  {
    OT_UNKNOWN,
    OT_SIMPLE_OBJECT,
    OT_ARRAY,
    OT_NUMBER_ARRAY,
    OT_OBJECT_ARRAY,
    OT_STRING_ARRAY,
    OT_ESCAPED_STRING_ARRAY
  };

  class JsonObjectDelegate final
  {
  public:
    explicit JsonObjectDelegate(
      JsonObject& jo,
      Writer& writer,
      const bool is_array);

  private:
    friend class JsonObject;
    JsonObject* j_obj_;
    Writer* ptr_writer_;
    bool is_array_;
  };

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  explicit JsonObject(JsonObjectDelegate&& delegate);

  ~JsonObject();

  // Add attributes into JsonObject
  JsonObject& add_string(
    const String::SubString& name,
    const String::SubString& value);

  template<class T>
  JsonObject& add_as_string(
    const String::SubString& name,
    const T& value);

  JsonObject& add_escaped_string(
    const String::SubString& name,
    const String::SubString& value);

  JsonObject& add_escaped_string_if_non_empty(
    const String::SubString& name,
    const String::SubString& value);

  JsonObject& add_opt_escaped_string(
    const String::SubString& name,
    const String::SubString& value,
    const bool need_escape);

  template<class T>
  JsonObject& add_number(
    const String::SubString& name,
    const T& value);

  // Use for in RunTime PreFormattedType (YandexFormatter)
  // added in BiddingFrontend only in object, not array
  template<class T>
  JsonObject& add(
    const String::SubString& name,
    const T& value);

  JsonObjectDelegate add_object(
    const String::SubString& name);

  // Array functions
  JsonObjectDelegate add_array(
    const String::SubString& name);

  JsonObject& add_string(
    const String::SubString& value);

  JsonObject& add_escaped_string(const String::SubString& value);

  JsonObject& add_opt_escaped_string(
    const String::SubString& value,
    const bool need_escape);

  template<class T>
  JsonObject& add_number(const T& value);

  JsonObjectDelegate add_object();

protected:
  explicit JsonObject(
    std::string& out,
    const bool is_object,
    const std::size_t initial_stack_capacity,
    const std::size_t initial_buffer_capacity);

  // Check on add elements into object
  void check(const String::SubString& name);

  // Check on add elements into array
  void check(const ObjectType required_type);

private:
  StringBufferOpt buffer_;

  WriterOpt writer_;

  Writer* ptr_writer_ = nullptr;

  std::string* out_string_ = nullptr;

  ObjectType type_ = OT_UNKNOWN;

  size_t opened_child_object_ = 0;

  JsonObject* parent_ = nullptr;
};

class JsonFormatter final : public JsonObject
{
public:
  explicit JsonFormatter(
    std::string& out,
    const bool is_object = true,
    const std::size_t initial_stack_capacity = kInitialStackCapacity,
    const std::size_t initial_buffer_capacity = rapidjson::StringBuffer::kDefaultCapacity);

  static constexpr size_t kInitialStackCapacity = 32;
};

inline
JsonObject::JsonObjectDelegate::JsonObjectDelegate(
  JsonObject& jo,
  Writer& writer,
  const bool is_array)
  : j_obj_(&jo),
    ptr_writer_(&writer),
    is_array_(is_array)
{
}

inline
JsonFormatter::JsonFormatter(
  std::string& out,
  const bool is_object,
  const std::size_t initial_stack_capacity,
  const std::size_t initial_buffer_capacity)
  : JsonObject(out, is_object, initial_stack_capacity, initial_buffer_capacity)
{
}

inline
JsonObject::JsonObject(
  std::string& out,
  const bool is_object,
  const std::size_t initial_stack_capacity,
  const std::size_t initial_buffer_capacity)
  : buffer_(std::in_place, nullptr, initial_buffer_capacity),
    writer_(std::in_place, *buffer_, nullptr, initial_stack_capacity),
    ptr_writer_(&writer_.value()),
    out_string_(&out),
    type_(is_object ? OT_SIMPLE_OBJECT : OT_ARRAY),
    opened_child_object_(0),
    parent_(nullptr)
{
  if (type_ == OT_SIMPLE_OBJECT)
  {
    ptr_writer_->StartObject();
  }
  else
  {
    ptr_writer_->StartArray();
  }
}

inline
JsonObject::JsonObject(
  JsonObjectDelegate&& delegate)
  : ptr_writer_(delegate.ptr_writer_),
    type_(delegate.is_array_ ? OT_ARRAY : OT_SIMPLE_OBJECT),
    opened_child_object_(0),
    parent_(delegate.j_obj_)
{
  parent_->opened_child_object_ += 1;
  if (type_ == OT_SIMPLE_OBJECT)
  {
    ptr_writer_->StartObject();
  }
  else
  {
    ptr_writer_->StartArray();
  }
}

inline
JsonObject::~JsonObject()
{
  try
  {
    if (parent_)
    {
      parent_->opened_child_object_ -= 1;
    }

    if (type_ == OT_SIMPLE_OBJECT)
    {
      ptr_writer_->EndObject();
    }
    else
    {
      ptr_writer_->EndArray();
    }

    if (!parent_)
    {
      out_string_->assign(
        buffer_->GetString(),
        buffer_->GetSize());
    }
  }
  catch (...)
  {
  }
}

inline
JsonObject& JsonObject::add_string(
  const String::SubString& name,
  const String::SubString& value)
{
  check(name);
  ptr_writer_->Key(name.data(), name.length());
  ptr_writer_->String(value.data(), value.length());

  return *this;
}

template<class T>
inline
JsonObject& JsonObject::add_as_string(
  const String::SubString& name,
  const T& value)
{
  check(name);
  ptr_writer_->Key(name.data(), name.length());

  if constexpr (std::is_same_v<T, bool>)
  {
    if (value)
    {
      ptr_writer_->String("true", 4);
    }
    else
    {
      ptr_writer_->String("false", 5);
    }
  }
  else if constexpr (std::is_same_v<T, std::string>
                  || std::is_same_v<T, String::SubString>)
  {
    ptr_writer_->String(value.data(), value.length());
  }
  else if constexpr (std::is_same_v<std::decay_t<T>, char*>)
  {
    ptr_writer_->String(value, std::strlen(value));
  }
  else if constexpr (std::is_arithmetic_v<T>)
  {
    const auto value_string = std::to_string(value);
    ptr_writer_->String(value_string.data(), value_string.length());
  }
  else if constexpr (Internal::has_member_str_v<T>)
  {
    const auto str = value.str();
    ptr_writer_->String(str.data(), str.length());
  }
  else
  {
    std::ostringstream stream;
    stream << value;
    const auto str = stream.str();
    ptr_writer_->String(str.data(), str.length());
  }

  return *this;
}

inline
JsonObject& JsonObject::add_escaped_string(
  const String::SubString& name,
  const String::SubString& value)
{
  check(name);
  ptr_writer_->Key(name.data(), name.length());
  ptr_writer_->String(value.data(), value.length());

  return *this;
}

inline
JsonObject& JsonObject::add_escaped_string_if_non_empty(
  const String::SubString& name,
  const String::SubString& value)
{
  if(!value.empty())
  {
    add_escaped_string(name, value);
  }

  return *this;
}

inline
JsonObject& JsonObject::add_opt_escaped_string(
  const String::SubString& name,
  const String::SubString& value,
  const bool need_escape)
{
  return need_escape ? add_escaped_string(name, value) :
    add_string(name, value);
}

template<class T>
inline
JsonObject& JsonObject::add_number(
  const String::SubString& name,
  const T& value)
{
  check(name);
  ptr_writer_->Key(name.data(), name.length());

  if constexpr (std::is_floating_point_v<T>)
  {
    ptr_writer_->Double(value);
  }
  else if constexpr (std::is_integral_v<T>
                  && std::is_signed_v<T>)
  {
    if constexpr (std::numeric_limits<T>::digits <= std::numeric_limits<int>::digits)
    {
      ptr_writer_->Int(value);
    }
    else
    {
      ptr_writer_->Int64(value);
    }
  }
  else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
  {
    if constexpr (std::numeric_limits<T>::digits <= std::numeric_limits<unsigned>::digits)
    {
      ptr_writer_->Uint(value);
    }
    else
    {
      ptr_writer_->Uint64(value);
    }
  }
  else if constexpr (Internal::has_member_str_v<T>)
  {
    const auto str = value.str();
    ptr_writer_->RawNumber(str.data(), str.length());
  }
  else
  {
    std::ostringstream stream;
    stream << value;
    const auto str = stream.str();
    ptr_writer_->String(str.data(), str.length());
  }

  return *this;
}

template<typename T>
inline
JsonObject& JsonObject::add(
  const String::SubString& name,
  const T& value)
{
  check(name);
  ptr_writer_->Key(name.data(), name.length());

  if constexpr (std::is_same_v<T, bool>)
  {
    ptr_writer_->Bool(value);
  }
  else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, String::SubString>)
  {
    ptr_writer_->String(value.data(), value.length());
  }
  else if constexpr (std::is_same_v<std::decay_t<T>, char*>)
  {
    ptr_writer_->String(value, std::strlen(value));
  }
  else if constexpr (std::is_floating_point_v<T>)
  {
    ptr_writer_->Double(value);
  }
  else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
  {
    if constexpr (std::numeric_limits<T>::digits <= std::numeric_limits<int>::digits)
    {
      ptr_writer_->Int(value);
    }
    else
    {
      ptr_writer_->Int64(value);
    }
  }
  else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
  {
    if constexpr (std::numeric_limits<T>::digits <= std::numeric_limits<unsigned>::digits)
    {
      ptr_writer_->Uint(value);
    }
    else
    {
      ptr_writer_->Uint64(value);
    }
  }
  else if constexpr (Internal::has_member_str_v<T>)
  {
    const auto str = value.str();
    ptr_writer_->String(str.data(), str.length());
  }
  else
  {
    std::stringstream stream;
    stream << value;
    const auto str = stream.str();
    ptr_writer_->String(str.data(), str.length());
  }

  return *this;
}

inline
JsonObject::JsonObjectDelegate JsonObject::add_object(
  const String::SubString& name)
{
  check(name);
  ptr_writer_->Key(name.data(), name.length());
  return JsonObjectDelegate(*this, *ptr_writer_, false);
}

inline
JsonObject::JsonObjectDelegate JsonObject::add_array(
  const String::SubString& name)
{
  check(name);
  ptr_writer_->Key(name.data(), name.length());
  return JsonObjectDelegate(*this, *ptr_writer_, true);
}

inline
JsonObject& JsonObject::add_string(
  const String::SubString& value)
{
  check(OT_STRING_ARRAY);
  ptr_writer_->String(value.data(), value.length());

  return *this;
}

inline
JsonObject& JsonObject::add_escaped_string(
  const String::SubString& value)
{
  check(OT_ESCAPED_STRING_ARRAY);
  ptr_writer_->String(value.data(), value.length());

  return *this;
}

inline
JsonObject& JsonObject::add_opt_escaped_string(
  const String::SubString& value,
  const bool need_escape)
{
  return need_escape? add_escaped_string(value): add_string(value);
}

template<class T>
inline
JsonObject& JsonObject::add_number(const T& value)
{
  check(OT_NUMBER_ARRAY);

  if constexpr (std::is_floating_point_v<T>)
  {
    ptr_writer_->Double(value);
  }
  else if constexpr (std::is_integral_v<T>
                  && std::is_signed_v<T>)
  {
    if constexpr (std::numeric_limits<T>::digits <= std::numeric_limits<int>::digits)
    {
      ptr_writer_->Int(value);
    }
    else
    {
      ptr_writer_->Int64(value);
    }
  }
  else if constexpr (std::is_integral_v<T>
                  && std::is_unsigned_v<T>)
  {
    if constexpr (std::numeric_limits<T>::digits <= std::numeric_limits<unsigned>::digits)
    {
      ptr_writer_->Uint(value);
    }
    else
    {
      ptr_writer_->Uint64(value);
    }
  }
  else if constexpr (Internal::has_member_str_v<T>)
  {
    const auto str = value.str();
    ptr_writer_->RawNumber(str.data(), str.length());
  }
  else
  {
    std::stringstream stream;
    stream << value;
    const auto str = stream.str();
    ptr_writer_->String(str.data(), str.length());
  }

  return *this;
}

inline
JsonObject::JsonObjectDelegate JsonObject::add_object()
{
  check(OT_OBJECT_ARRAY);
  return JsonObjectDelegate(*this, *ptr_writer_, false);
}

inline
void JsonObject::check(const String::SubString& name)
{
  if (opened_child_object_)
  {
    Stream::Error stream;
    stream << FNS
           << " You can not add data to parent object, "
              "before close all child objects. Error occured on add: \""
           << name
           << "\"";
    throw Exception(stream.str());
  }

  if (type_ != OT_SIMPLE_OBJECT)
  {
    Stream::Error stream;
    stream << FNS
           << " Json Object function calld to for Json Array. "
              "Error occured on call for element: \""
           << name
           << "\"";
    throw Exception(stream.str());
  }
}

inline
void JsonObject::check(const ObjectType required_type)
{
  if (opened_child_object_)
  {
    Stream::Error stream;
    stream << FNS
           << " You can not add data to parent array, "
              "before close all child objects.";
    throw Exception(stream);
  }

  if (type_ == OT_SIMPLE_OBJECT)
  {
    Stream::Error stream;
    stream << FNS
           << " Json Array function called to Json Simple Object (not array)";
    throw Exception(stream);
  }

  if (type_ == OT_ARRAY)
  {
    type_ = required_type;
  }
  else if (type_ != required_type)
  {
    Stream::Error stream;
    stream << FNS
           << " You try add elements with different type into array";
    throw Exception(stream);
  }
}

} // namespace AdServer::Commons