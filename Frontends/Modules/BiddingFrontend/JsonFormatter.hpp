#ifndef COMMONS_JSON_FORMATTER_HPP__
#define COMMONS_JSON_FORMATTER_HPP__

#include <ostream>
#include <String/SubString.hpp>
#include <String/StringManip.hpp>
#include <Stream/MemoryStream.hpp>
#include <Generics/Function.hpp>

namespace AdServer
{
namespace Commons
{
  // Write Bid Response in JSON format in stream on the fly.

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
    add_boolean(const String::SubString& name, bool value);

    JsonObject&
    add_string(const String::SubString& name, const String::SubString& value);

    template<typename AsStringOutType>
    JsonObject&
    add_as_string(const String::SubString& name, const AsStringOutType& value);

    JsonObject&
    add_escaped_string(const String::SubString& name, const String::SubString& value);

    JsonObject&
    add_escaped_string_if_non_empty(const String::SubString& name, const String::SubString& value);

    JsonObject&
    add_opt_escaped_string(
      const String::SubString& name,
      const String::SubString& value,
      bool need_escape);

    template<typename NumberType>
    JsonObject&
    add_number(const String::SubString& name, const NumberType& value);

    // Use for in RunTime PreFormattedType (YandexFormatter) 
    //   added in BiddingFrontend only in object, not array
    template<typename PreFormattedType>
    JsonObject&
    add(const String::SubString& name, const PreFormattedType& value);

    JsonObjectDelegate
    add_object(const String::SubString& name);

    // Array functions
    JsonObjectDelegate
    add_array(const String::SubString& name);

    // Add elements into JsonArray
    template<typename NumberType>
    JsonObject&
    add_number(const NumberType& value);

    JsonObject&
    add_string(const String::SubString& value);

    JsonObject&
    add_escaped_string(const String::SubString& value);

    JsonObject&
    add_opt_escaped_string(
      const String::SubString& value,
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
  
    JsonObject(std::ostream& out);

    // Check on add elements into object
    void
    check_(const String::SubString& name);

    // Check on add elements into array
    void
    check_(ObjectType required_type);

    void
    comma_();

    void
    name_(const String::SubString& name);

  private:
    ObjectType type_;
    std::ostream& out_stream_;
    bool empty_;
    size_t opened_child_object_;
    JsonObject* parent_;
  };
  
  class JsonFormatter: public JsonObject
  {
  public:
    JsonFormatter(std::ostream& out);
  };
}
}

// Impl
namespace AdServer
{
namespace Commons
{
  inline
  JsonObject::JsonObjectDelegate::JsonObjectDelegate(JsonObject& jo, bool array)
  : j_obj_(&jo), array_(array)
  {}

  inline
  JsonFormatter::JsonFormatter(std::ostream& out)
  : JsonObject(out)
  {}

  inline
  JsonObject::JsonObject(JsonObjectDelegate&& delegate)
  : type_(delegate.array_ ? OT_ARRAY : OT_SIMPLE_OBJECT),
    out_stream_(delegate.j_obj_->out_stream_),
    empty_(true),
    opened_child_object_(0),
    parent_(delegate.j_obj_)
  {
    ++parent_->opened_child_object_;
    out_stream_ << (type_ == OT_SIMPLE_OBJECT ? "{" : "[");
  }

  inline
  JsonObject::JsonObject(std::ostream& out)
  : type_(OT_SIMPLE_OBJECT),
    out_stream_(out),
    empty_(true),
    opened_child_object_(0),
    parent_(0)
  {
    out_stream_ << "{";
  }

  inline
  JsonObject::~JsonObject()
  {
    if (parent_)
    {
      --parent_->opened_child_object_;
    }
    out_stream_ << (type_ == OT_SIMPLE_OBJECT ? "}" : "]");
  }

  inline
  void
  JsonObject::check_(const String::SubString& name)
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
      out_stream_ << ", ";
    }
    empty_ = false;
  }

  inline
  void
  JsonObject::name_(const String::SubString& name)
  {
    out_stream_ <<  "\"" << name << "\": ";
  }

  inline
  JsonObject::JsonObjectDelegate
  JsonObject::add_object(const String::SubString& name)
  {
    check_(name);
    comma_();
    name_(name);
    return JsonObjectDelegate(*this, false);
  }

  inline
  JsonObject::JsonObjectDelegate
  JsonObject::add_array(const String::SubString& name)
  {
    check_(name);
    comma_();
    name_(name);
    return JsonObjectDelegate(*this, true);
  }

  // Add into object
  inline
  JsonObject&
  JsonObject::add_boolean(const String::SubString& name, bool value)
  {
    check_(name);
    comma_();
    name_(name);
    out_stream_ << (value ? "true" : "false");
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_string(const String::SubString& name, const String::SubString& value)
  {
    check_(name);
    comma_();
    name_(name);
    out_stream_ << "\"" << value << "\"";
    return *this;
  }

  template<typename AsStringOutType>
  inline
  JsonObject&
  JsonObject::add_as_string(const String::SubString& name, const AsStringOutType& value)
  {
    check_(name);
    comma_();
    name_(name);
    out_stream_ << "\"" << value << "\"";
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_escaped_string(const String::SubString& name, const String::SubString& value)
  {
    check_(name);
    comma_();
    name_(name);
    out_stream_ << "\"" << String::StringManip::json_escape(value) << "\"";
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_escaped_string_if_non_empty(
    const String::SubString& name, const String::SubString& value)
  {
    if(!value.empty())
    {
      add_escaped_string(name, value);
    }

    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_opt_escaped_string(
    const String::SubString& name,
    const String::SubString& value,
    bool need_escape)
  {
    return need_escape ? add_escaped_string(name, value) :
      add_string(name, value);
  }

  template<typename Number>
  inline
  JsonObject&
  JsonObject::add_number(const String::SubString& name, const Number& value)
  {
    check_(name);
    comma_();
    name_(name);
    out_stream_ << value;
    return *this;
  }

  template<typename PreFormattedType>
  inline
  JsonObject&
  JsonObject::add(const String::SubString& name, const PreFormattedType& value)
  {
    // Same code
    return add_number(name, value);
  }

  // Add into array
  inline
  JsonObject&
  JsonObject::add_string(const String::SubString& value)
  {
    check_(OT_STRING_ARRAY);
    comma_();
    out_stream_ << "\"" << value << "\"";
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_escaped_string(const String::SubString& value)
  {
    check_(OT_ESCAPED_STRING_ARRAY);
    comma_();
    out_stream_ << "\"" << String::StringManip::json_escape(value) << "\"";
    return *this;
  }

  inline
  JsonObject&
  JsonObject::add_opt_escaped_string(
    const String::SubString& value,
    bool need_escape)
  {
    return need_escape?
      add_escaped_string(value):
        add_string(value);
  }

  template<typename Number>
  inline
  JsonObject&
  JsonObject::add_number(const Number& value)
  {
    check_(OT_NUMBER_ARRAY);
    comma_();
    out_stream_ << value;
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

}
}

#endif //COMMONS_JSON_FORMATTER_HPP__

