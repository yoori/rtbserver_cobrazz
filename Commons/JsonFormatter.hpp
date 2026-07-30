#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <String/StringManip.hpp>
#include <Stream/MemoryStream.hpp>
#include <Generics/Function.hpp>

namespace AdServer::Commons
{
  namespace JsonFormatterDetail
  {
    template<typename Value, typename = void>
    struct HasStrMethod: std::false_type
    {};

    template<typename Value>
    struct HasStrMethod<
      Value,
      std::void_t<decltype(std::declval<const Value&>().str())>>:
      std::true_type
    {};

    template<typename Value, typename = void>
    struct HasAppendToMethod: std::false_type
    {};

    template<typename Value>
    struct HasAppendToMethod<
      Value,
      std::void_t<decltype(
        std::declval<const Value&>().append_to(std::declval<std::string&>()))>>:
      std::true_type
    {};

    template<typename Value>
    constexpr bool dependent_false = false;

    inline constexpr std::string_view SIMPLE_OBJECT_BEGIN{"{"};
    inline constexpr std::string_view SIMPLE_OBJECT_END{"}"};
    inline constexpr std::string_view ARRAY_BEGIN{"["};
    inline constexpr std::string_view ARRAY_END{"]"};
    inline constexpr std::string_view COMMA_SEPARATOR{", "};
    inline constexpr std::string_view NAME_SEPARATOR{"\": "};
    inline constexpr std::string_view TRUE_VALUE{"true"};
    inline constexpr std::string_view FALSE_VALUE{"false"};
  }

  // Write Bid Response in JSON format into string on the fly.

  // Possible split JsonObject to JsonObject and JsonArray
  class JsonObject
  {
    class JsonObjectDelegate
    {
    public:
      JsonObjectDelegate(JsonObject& jo, bool array = false);
    private:
      friend class JsonObject;
      JsonObject* j_obj_;
      bool array_;
    };

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    JsonObject(JsonObjectDelegate&&);

    ~JsonObject();

    // Add attributes into JsonObject
    JsonObject&
    add_boolean(std::string_view name, bool value);

    JsonObject&
    add_string(std::string_view name, std::string_view value);

    template<typename AsStringOutType>
    JsonObject&
    add_as_string(std::string_view name, const AsStringOutType& value);

    JsonObject&
    add_escaped_string(std::string_view name, std::string_view value);

    JsonObject&
    add_escaped_string_if_non_empty(
      std::string_view name,
      std::string_view value);

    JsonObject&
    add_opt_escaped_string(
      std::string_view name,
      std::string_view value,
      bool need_escape);

    template<typename NumberType>
    JsonObject&
    add_number(std::string_view name, const NumberType& value);

    // Use for in RunTime PreFormattedType (YandexFormatter)
    //   added in BiddingFrontend only in object, not array
    template<typename PreFormattedType>
    JsonObject&
    add(std::string_view name, const PreFormattedType& value);

    JsonObjectDelegate
    add_object(std::string_view name);

    // Array functions
    JsonObjectDelegate
    add_array(std::string_view name);

    // Add elements into JsonArray
    template<typename NumberType>
    JsonObject&
    add_number(const NumberType& value);

    JsonObject&
    add_string(std::string_view value);

    JsonObject&
    add_escaped_string(std::string_view value);

    JsonObject&
    add_opt_escaped_string(
      std::string_view value,
      bool need_escape);

    JsonObjectDelegate
    add_object();

  private:
    // can't be copied
    JsonObject(const JsonObject&);
    JsonObject& operator=(const JsonObject&);

  protected:
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

    JsonObject(std::string& out);

    // Check on add elements into object
    void
    check_(std::string_view name);

    // Check on add elements into array
    void
    check_(ObjectType required_type);

    void
    comma_();

    void
    name_(std::string_view name);

    template<std::size_t Size>
    void
    append_(const char (&value)[Size]);

    void
    append_(char value);

    void
    append_(std::string_view value);

    template<typename Value>
    void
    append_value_(const Value& value);

  private:
    ObjectType type_;
    std::string* out_string_;
    bool empty_;
    size_t opened_child_object_;
    JsonObject* parent_;
  };

  class JsonFormatter: public JsonObject
  {
  public:
    JsonFormatter(std::string& out);
  };
} // namespace AdServer::Commons

// Impl
namespace AdServer::Commons
{
  inline
  JsonObject::JsonObjectDelegate::JsonObjectDelegate(JsonObject& jo, bool array)
    : j_obj_(&jo), array_(array)
  {}

  inline
  JsonFormatter::JsonFormatter(std::string& out)
    : JsonObject(out)
  {}

  inline
  JsonObject::JsonObject(JsonObjectDelegate&& delegate)
    : type_(delegate.array_ ? OT_ARRAY : OT_SIMPLE_OBJECT),
      out_string_(delegate.j_obj_->out_string_),
      empty_(true),
      opened_child_object_(0),
      parent_(delegate.j_obj_)
  {
    ++parent_->opened_child_object_;
    append_(
      type_ == OT_SIMPLE_OBJECT ?
        JsonFormatterDetail::SIMPLE_OBJECT_BEGIN :
        JsonFormatterDetail::ARRAY_BEGIN);
  }

  inline
  JsonObject::JsonObject(std::string& out)
    : type_(OT_SIMPLE_OBJECT),
      out_string_(&out),
      empty_(true),
      opened_child_object_(0),
      parent_(0)
  {
    append_(JsonFormatterDetail::SIMPLE_OBJECT_BEGIN);
  }

  inline
  JsonObject::~JsonObject()
  {
    if (parent_)
    {
      --parent_->opened_child_object_;
    }
    append_(
      type_ == OT_SIMPLE_OBJECT ?
        JsonFormatterDetail::SIMPLE_OBJECT_END :
        JsonFormatterDetail::ARRAY_END);
  }

  inline
  void
  JsonObject::check_(std::string_view name)
  {
    if (opened_child_object_)
    {
      Stream::Error ostr;
      ostr << FNS << " You can not add data to parent object, "
        "before close all child objects. Error occured on add: \"" << name << "\"";
      throw Exception(ostr.str());
    }

    if (type_ != OT_SIMPLE_OBJECT)
    {
      Stream::Error ostr;
      ostr << FNS << " Json Object function calld to for Json Array. "
        "Error occured on call for element: \"" << name << "\"";
      throw Exception(ostr.str());
    }
  }

  inline
  void
  JsonObject::check_(ObjectType required_type)
  {
    if (opened_child_object_)
    {
      Stream::Error ostr;
      ostr << FNS << " You can not add data to parent array, "
        "before close all child objects.";
      throw Exception(ostr.str());
    }

    if (type_ == OT_SIMPLE_OBJECT)
    {
      Stream::Error ostr;
      ostr << FNS << " Json Array function called to Json Simple Object (mot array)";
      throw Exception(ostr.str());
    }

    if (type_ == OT_ARRAY)
    {
      type_ = required_type;
    }
    else if (type_ != required_type)
    {
      Stream::Error ostr;
      ostr << FNS << " You try add elements with different type into array";
      throw Exception(ostr.str());
    }
  }

  inline
  void
  JsonObject::comma_()
  {
    if (!empty_)
    {
      append_(JsonFormatterDetail::COMMA_SEPARATOR);
    }
    empty_ = false;
  }

  inline
  void
  JsonObject::name_(std::string_view name)
  {
    append_('"');
    append_(name);
    append_(JsonFormatterDetail::NAME_SEPARATOR);
  }

  template<std::size_t Size>
  inline
  void
  JsonObject::append_(const char (&value)[Size])
  {
    out_string_->append(value, Size - 1);
  }

  inline
  void
  JsonObject::append_(char value)
  {
    out_string_->push_back(value);
  }

  inline
  void
  JsonObject::append_(std::string_view value)
  {
    out_string_->append(value.data(), value.size());
  }

  template<typename Value>
  inline
  void
  JsonObject::append_value_(const Value& value)
  {
    using ValueType = std::decay_t<Value>;

    if constexpr (
      std::is_integral_v<ValueType> &&
      !std::is_same_v<ValueType, bool>)
    {
      char buf[std::numeric_limits<ValueType>::digits10 + 3];
      const std::size_t len = String::StringManip::int_to_str(
        value,
        buf,
        sizeof(buf));
      append_(std::string_view(buf, len));
    }
    else if constexpr (std::is_enum_v<ValueType>)
    {
      append_value_(static_cast<std::underlying_type_t<ValueType>>(value));
    }
    else if constexpr (JsonFormatterDetail::HasAppendToMethod<ValueType>::value)
    {
      value.append_to(*out_string_);
    }
    else if constexpr (std::is_convertible_v<const Value&, std::string_view>)
    {
      append_(std::string_view(value));
    }
    else if constexpr (JsonFormatterDetail::HasStrMethod<ValueType>::value)
    {
      append_value_(value.str());
    }
    else
    {
      static_assert(
        JsonFormatterDetail::dependent_false<ValueType>,
        "Unsupported JsonFormatter value type");
    }
  }

  inline
  JsonObject::JsonObjectDelegate
  JsonObject::add_object(std::string_view name)
  {
    check_(name);
    comma_();
    name_(name);
    return JsonObjectDelegate(*this, false);
  }

  inline
  JsonObject::JsonObjectDelegate
  JsonObject::add_array(std::string_view name)
  {
    check_(name);
    comma_();
    name_(name);
    return JsonObjectDelegate(*this, true);
  }

  // Add into object
  inline
  JsonObject&
  JsonObject::add_boolean(std::string_view name, bool value)
  {
    check_(name);
    comma_();
    name_(name);
    if (value)
    {
      append_(JsonFormatterDetail::TRUE_VALUE);
    }
    else
    {
      append_(JsonFormatterDetail::FALSE_VALUE);
    }
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_string(std::string_view name, std::string_view value)
  {
    check_(name);
    comma_();
    name_(name);
    append_('"');
    append_(value);
    append_('"');
    return *this;
  }

  template<typename AsStringOutType>
  inline
  JsonObject&
  JsonObject::add_as_string(std::string_view name, const AsStringOutType& value)
  {
    check_(name);
    comma_();
    name_(name);
    append_('"');
    append_value_(value);
    append_('"');
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_escaped_string(std::string_view name, std::string_view value)
  {
    check_(name);
    comma_();
    name_(name);
    append_('"');
    String::StringManip::json_escape_append(*out_string_, value);
    append_('"');
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_escaped_string_if_non_empty(
    std::string_view name,
    std::string_view value)
  {
    if (!value.empty())
    {
      add_escaped_string(name, value);
    }

    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_opt_escaped_string(
    std::string_view name,
    std::string_view value,
    bool need_escape)
  {
    return need_escape ? add_escaped_string(name, value) : add_string(name, value);
  }

  template<typename Number>
  inline
  JsonObject&
  JsonObject::add_number(std::string_view name, const Number& value)
  {
    check_(name);
    comma_();
    name_(name);
    append_value_(value);
    return *this;
  }

  template<typename PreFormattedType>
  inline
  JsonObject&
  JsonObject::add(std::string_view name, const PreFormattedType& value)
  {
    // Same code
    return add_number(name, value);
  }

  // Add into array
  inline
  JsonObject&
  JsonObject::add_string(std::string_view value)
  {
    check_(OT_STRING_ARRAY);
    comma_();
    append_('"');
    append_(value);
    append_('"');
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_escaped_string(std::string_view value)
  {
    check_(OT_ESCAPED_STRING_ARRAY);
    comma_();
    append_('"');
    String::StringManip::json_escape_append(*out_string_, value);
    append_('"');
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_opt_escaped_string(
    std::string_view value,
    bool need_escape)
  {
    return need_escape ? add_escaped_string(value) : add_string(value);
  }

  template<typename Number>
  inline
  JsonObject&
  JsonObject::add_number(const Number& value)
  {
    check_(OT_NUMBER_ARRAY);
    comma_();
    append_value_(value);
    return *this;
  }

  inline
  JsonObject::JsonObjectDelegate
  JsonObject::add_object()
  {
    check_(OT_OBJECT_ARRAY);
    comma_();
    return JsonObjectDelegate(*this, false);
  }
} // namespace AdServer::Commons
