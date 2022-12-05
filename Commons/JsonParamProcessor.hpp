#ifndef COMMONS_JSONPARAMPROCESSOR_HPP_
#define COMMONS_JSONPARAMPROCESSOR_HPP_

#include <math.h>
#include <type_traits>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Generics/GnuHashTable.hpp>
#include <Commons/Gason.hpp>

namespace AdServer
{
namespace Commons
{
  // JsonParamProcessor
  template<typename RequestInfoType>
  class JsonParamProcessor:
    public ReferenceCounting::AtomicImpl
  {
  public:
    virtual void process(
      RequestInfoType& request_info,
      const JsonValue& value) const = 0;

  protected:
    virtual
    ~JsonParamProcessor() noexcept {}
  };

  // JsonStringParamProcessor
  template<typename RequestInfoType>
  struct JsonStringParamProcessor:
    public JsonParamProcessor<RequestInfoType>
  {
    JsonStringParamProcessor(
      std::string RequestInfoType::* field)
      noexcept;

    virtual void
    process(RequestInfoType& request_info,
      const JsonValue& value) const;

  protected:
    virtual ~JsonStringParamProcessor() noexcept
    {}

  protected:
    std::string RequestInfoType::* field_;
  };  

  // JsonNumberParamProcessor
  template<typename RequestInfoType, typename NumberType>
  struct JsonNumberParamProcessor:
    public JsonParamProcessor<RequestInfoType>
  {
    JsonNumberParamProcessor(
      NumberType RequestInfoType::* field)
      noexcept;

    virtual void
    process(RequestInfoType& request_info,
      const JsonValue& value) const;

  protected:
    virtual ~JsonNumberParamProcessor() noexcept
    {}

  protected:
    NumberType RequestInfoType::* field_;
  };

  // JsonDoubleParamProcessor
  template<typename RequestInfoType, typename DecimalType>
  struct JsonDecimalParamProcessor:
    public JsonParamProcessor<RequestInfoType>
  {
    JsonDecimalParamProcessor(
      DecimalType RequestInfoType::* field,
      Generics::DecimalMulRemainder round_type = Generics::DMR_ROUND)
      : field_(field),
        round_type_(round_type)
    {}

    virtual void
    process(
      RequestInfoType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_STRING)
      {
        std::string str = value.toString();

        if (!str.empty())
        {
          context.*field_ = Commons::extract_decimal<DecimalType>(str, round_type_);
        }
      }
      else if(value.getTag() == JSON_TAG_NUMBER)
      {
        context.*field_ = value.toDecimal<DecimalType>(round_type_);
      }
    }

  protected:
    virtual ~JsonDecimalParamProcessor() noexcept
    {}

  protected:
    DecimalType RequestInfoType::* field_;
    Generics::DecimalMulRemainder round_type_;
  };

  // JsonCompositeParamProcessor
  template<typename RequestInfoType>
  class JsonCompositeParamProcessor:
    public JsonParamProcessor<RequestInfoType>
  {
  public:
    JsonCompositeParamProcessor();

    JsonCompositeParamProcessor(bool RequestInfoType::* field);

    virtual void
    process(RequestInfoType& context,
      const JsonValue& value) const;

    void
    add_processor(
      const Generics::SubStringHashAdapter& key,
      JsonParamProcessor<RequestInfoType>* processor);

  protected:
    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter,
      ReferenceCounting::SmartPtr<JsonParamProcessor<RequestInfoType> > >
      JsonParamProcessorMap;

  protected:
    virtual ~JsonCompositeParamProcessor() noexcept
    {}

  private:
    bool field_installed_;
    bool RequestInfoType::* field_;
    JsonParamProcessorMap sub_processors_;
  };

  template<typename ParamType>
  struct DefaultConverter: public std::unary_function<ParamType, ParamType>
  {
    ParamType& operator()(ParamType& arg)
    {
      return arg;
    }
  };

  // JsonArrayParamProcessor
  template<
    typename RequestInfoType,
    typename ConverterType = DefaultConverter<RequestInfoType> >
  class JsonArrayParamProcessor:
    public JsonParamProcessor<RequestInfoType>
  {
  public:
    typedef typename std::remove_reference<typename ConverterType::result_type>::type
      SubRequestInfoType;

    JsonArrayParamProcessor(
      JsonParamProcessor<SubRequestInfoType>* element_processor,
      std::size_t max_count = std::numeric_limits<std::size_t>::max())
      noexcept;

    virtual void
    process(
      RequestInfoType& context,
      const JsonValue& value) const;

  protected:
    virtual ~JsonArrayParamProcessor() noexcept
    {}

  protected:
    ReferenceCounting::SmartPtr<JsonParamProcessor<SubRequestInfoType> >
      element_processor_;
    std::size_t max_count_;
  };

  // JsonNumberArrayParamProcessor
  template<typename RequestInfoType, typename CollectionType>
  class JsonNumberArrayParamProcessor:
    public JsonParamProcessor<RequestInfoType>
  {
  public:
    JsonNumberArrayParamProcessor(
      CollectionType RequestInfoType::* field)
      noexcept;

    virtual void
    process(
      RequestInfoType& context,
      const JsonValue& value) const;

  protected:
    CollectionType RequestInfoType::* field_;
  };

  template<typename RequestInfoType, typename CollectionType>
  class JsonStringArrayParamProcessor:
    public JsonParamProcessor<RequestInfoType>
  {
  public:
    JsonStringArrayParamProcessor(
      CollectionType RequestInfoType::* field);

    virtual void
    process(
      RequestInfoType& context,
      const JsonValue& value) const;

  protected:
    CollectionType RequestInfoType::* field_;
  };
}
}

namespace AdServer
{
namespace Commons
{
  // JsonStringParamProcessor impl
  template<typename RequestInfoType>
  JsonStringParamProcessor<RequestInfoType>::JsonStringParamProcessor(
    std::string RequestInfoType::* field)
    noexcept
    : field_(field)
  {}

  template<typename RequestInfoType>
  void
  JsonStringParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const JsonValue& value) const
  {
    if(value.getTag() == JSON_TAG_NUMBER ||
       value.getTag() == JSON_TAG_STRING)
    {
      value.toString(request_info.*field_);
    }
  }

  // JsonNumberParamProcessor impl
  template<typename RequestInfoType, typename NumberType>
  JsonNumberParamProcessor<RequestInfoType, NumberType>::
  JsonNumberParamProcessor(
    NumberType RequestInfoType::* field)
    noexcept
    : field_(field)
  {}

  template<typename RequestInfoType, typename NumberType>
  void
  JsonNumberParamProcessor<RequestInfoType, NumberType>::
  process(RequestInfoType& request_info,
    const JsonValue& value) const
  {
    if(value.getTag() == JSON_TAG_NUMBER)
    {
      request_info.*field_ = value.toNumber();
    }
  }

  // JsonCompositeParamProcessor impl
  template<typename RequestInfoType>
  JsonCompositeParamProcessor<RequestInfoType>::
  JsonCompositeParamProcessor()
    : field_installed_(false), field_(0)
  {}

  template<typename RequestInfoType>
  JsonCompositeParamProcessor<RequestInfoType>::
  JsonCompositeParamProcessor(bool RequestInfoType::* field)
    : field_installed_(true),
      field_(field)
  {}

  template<typename RequestInfoType>
  void
  JsonCompositeParamProcessor<RequestInfoType>::
  process(RequestInfoType& request_info,
    const JsonValue& value) const
  {
    if(field_installed_)
    {
      request_info.*field_ = true;
    }

    if(value.getTag() == JSON_TAG_OBJECT)
    {
      for(JsonIterator it = begin(value); it != end(value); ++it)
      {
        typename JsonParamProcessorMap::const_iterator processor_it =
          sub_processors_.find(String::SubString(it->key));
        if(processor_it != sub_processors_.end())
        {
          processor_it->second->process(request_info, it->value);
        }
      }
    }
  }

  template<typename RequestInfoType>
  void
  JsonCompositeParamProcessor<RequestInfoType>::add_processor(
    const Generics::SubStringHashAdapter& key,
    JsonParamProcessor<RequestInfoType>* processor)
  {
    sub_processors_.insert(std::make_pair(
      key,
      ReferenceCounting::add_ref(processor)));
  }

  // JsonArrayParamProcessor impl
  template<typename RequestInfoType, typename ConverterType>
  JsonArrayParamProcessor<RequestInfoType, ConverterType>::
  JsonArrayParamProcessor(
    JsonParamProcessor<SubRequestInfoType>* element_processor,
    std::size_t max_count)
    noexcept
    : element_processor_(ReferenceCounting::add_ref(element_processor)),
      max_count_(max_count)
  {}

  template<typename RequestInfoType, typename ConverterType>
  void
  JsonArrayParamProcessor<RequestInfoType, ConverterType>::process(
    RequestInfoType& request_info,
    const JsonValue& value) const
  {
    if(value.getTag() == JSON_TAG_ARRAY)
    {
      ConverterType converter;
      std::size_t inx = 0;

      for(JsonIterator it = begin(value);
          it != end(value) && inx < max_count_;
          ++it, ++inx)
      {
        element_processor_->process(
          converter(request_info),
          it->value);
      }
    }
  }

  // JsonNumberArrayParamProcessor impl
  template<typename RequestInfoType, typename CollectionType>
  JsonNumberArrayParamProcessor<RequestInfoType, CollectionType>::
  JsonNumberArrayParamProcessor(
    CollectionType RequestInfoType::* field)
    noexcept
    : field_(field)
  {}

  template<typename RequestInfoType, typename CollectionType>
  void
  JsonNumberArrayParamProcessor<RequestInfoType, CollectionType>::process(
    RequestInfoType& context,
    const JsonValue& value) const
  {
    if(value.getTag() == JSON_TAG_ARRAY)
    {
      for(JsonIterator it = begin(value); it != end(value); ++it)
      {
        if(it->value.getTag() == JSON_TAG_NUMBER)
        {
          (context.*field_).insert(
            (context.*field_).end(),
            static_cast<typename CollectionType::value_type>(
              ::round(it->value.toNumber())));
        }
      }
    }
    else if(value.getTag() == JSON_TAG_NUMBER)
    {
      (context.*field_).insert(
        (context.*field_).end(),
        static_cast<typename CollectionType::value_type>(
          value.toNumber()));
    }
  }

  // JsonStringArrayParamProcessor
  template<typename RequestInfoType, typename CollectionType>
  JsonStringArrayParamProcessor<RequestInfoType, CollectionType>::
  JsonStringArrayParamProcessor(
    CollectionType RequestInfoType::* field)
    : field_(field)
  {}

  template<typename RequestInfoType, typename CollectionType>
  void
  JsonStringArrayParamProcessor<RequestInfoType, CollectionType>::
  process(
    RequestInfoType& context,
    const JsonValue& value) const
  {
    if(value.getTag() == JSON_TAG_ARRAY)
    {
      for(JsonIterator it = begin(value); it != end(value); ++it)
      {
        if(it->value.getTag() == JSON_TAG_STRING ||
          it->value.getTag() == JSON_TAG_NUMBER)
        {
          (context.*field_).insert((context.*field_).end(), it->value.toString());
        }
      }
    }
    else if(value.getTag() == JSON_TAG_STRING ||
      value.getTag() == JSON_TAG_NUMBER)
    {
      (context.*field_).insert((context.*field_).end(), value.toString());
    }
  }
}
}

#endif /*COMMONS_JSONPARAMPROCESSOR_HPP_*/
